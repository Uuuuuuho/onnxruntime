# -------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See License.txt in the project root for
# license information.
# --------------------------------------------------------------------------
from __future__ import annotations

import logging
from dataclasses import dataclass
from enum import Enum

import numpy as np
import onnx
import onnx.numpy_helper
from onnx import TensorProto
from onnx import onnx_pb as onnx_proto

from .base_quantizer import BaseQuantizer, QuantizationParams
from .calibrate import TensorData
from .quant_utils import (
    DEQUANT_OP_NAME,
    QUANT_OP_NAME,
    QuantizedValue,
    QuantizedValueType,
    __producer__,
    __version__,
    add_dequant_output_suffix,
    add_dequant_suffix,
    add_quant_input_suffix,
    add_quant_output_suffix,
    add_quant_suffix,
    compute_scale_zp,
    compute_scale_zp_float8,
    find_by_name,
    get_qmin_qmax_for_qType,
    ms_domain,
    tensor_proto_to_array,
)
from .registry import CreateQDQQuantizer


class QDQQuantTensorType(Enum):
    ACTIVATION = 0
    WEIGHT = 1
    BIAS = 2


@dataclass
class QDQQuantParamProvider:
    input_name: str
    node_name: str


class QDQTensorQuantInfo:
    def __init__(self, tensor_type=QDQQuantTensorType.ACTIVATION, quant_para_provider=None, axis=None, data_type=None):
        self.tensor_type = tensor_type
        self.quant_para_provider = quant_para_provider
        self.axis = axis
        self.is_shared = quant_para_provider is not None
        assert data_type is not None
        self.data_type = data_type


@dataclass
class QDQTensorQuantParams:
    original: QuantizationParams
    converted: QuantizationParams | None
    converted_recv_nodes: set[str] | None


@dataclass
class QDQQuantParamsInitializers:
    scale: TensorProto
    zero_point: TensorProto


@dataclass
class QDQTensorQuantParamsInitializers:
    original: QDQQuantParamsInitializers
    converted: QDQQuantParamsInitializers | None
    converted_recv_nodes: set[str] | None


@dataclass
class QDQTensorQuantizedValue:
    original: QuantizedValue
    converted: QuantizedValue | None
    converted_recv_nodes: set[str] | None

    def get_for_consumer(self, consumer_node_name) -> QuantizedValue:
        if self.converted is None:  # Quantized value is not converted, return original
            return self.original

        if self.converted_recv_nodes is None:  # All consumers receive the converted value
            return self.converted

        # Check if consumer node name is in the list of nodes that
        # receive the converted quantization value. If not, return the original value generated
        # by the tensor's producer.
        return self.converted if (consumer_node_name in self.converted_recv_nodes) else self.original


class QDQQuantizer(BaseQuantizer):
    def __init__(
        self,
        model,
        per_channel,
        reduce_range,
        weight_qType,
        activation_qType,
        tensors_range,
        nodes_to_quantize,
        nodes_to_exclude,
        op_types_to_quantize,
        extra_options=None,
    ):
        BaseQuantizer.__init__(
            self,
            model,
            per_channel,
            reduce_range,
            weight_qType,
            activation_qType,
            tensors_range,
            nodes_to_quantize,
            nodes_to_exclude,
            op_types_to_quantize,
            extra_options,
        )

        self.tensors_to_quantize = {}
        self.bias_to_quantize = []

        self.nodes_to_remove = []

        # Specific op types to exclude qdq quantization for their outputs.
        # In TRT, it's not recommended to quantize outputs for weighted ops such as Conv, Matmul, Gemm
        # because those ops may be followed by nodes that require high resolution inputs.
        # Adding QDQ for those ops' output may end up with worse accuracy.
        # So, we don't recommend to add QDQ to node's output under such condition.
        self.op_types_to_exclude_output_quantization = extra_options.get("OpTypesToExcludeOutputQuantization", [])

        # We do quantization on Dequantizelinear's input to remove Quantizelinear for weight as an optimization.
        # In some cases, for example QDQ BERT model for TensorRT, QDQ should always appear as a pair.
        # Therefore, we need to disable this optimization and add qdq pair to weight.
        self.add_qdq_pair_to_weight = extra_options.get("AddQDQPairToWeight", False)

        # Some scenarios do not need the bias quantized. For example, in the case of Quantization Aware Training,
        # quantizing the bias is not needed. This is because in QAT, all model parameters are expected to be in
        # floating point format. To that end, we can use the FakeQuant operator for weights and activations that
        # can always have QDQ pairs (by using AddQDQPairToWeight). But for biases in a quantized model, we can't use
        # FakeQuant because it only ever appears before a DQ (since it is quantized as int32).
        self.quantize_bias = extra_options.get("QuantizeBias", True)

        # The default behavior is that multiple nodes can share a QDQ pair as their inputs.
        # In TRT, QDQ pair can`t be shared between nodes, so it will create dedicated QDQ pairs for each node.
        self.dedicated_qdq_pair = extra_options.get("DedicatedQDQPair", False)
        self.tensor_to_its_receiving_nodes = {}

        # Let user set channel axis for specific op type and it's effective only when per channel quantization is supported and per_channel is True.
        self.qdq_op_type_per_channel_support_to_axis = extra_options.get("QDQOpTypePerChannelSupportToAxis", {})

        self.qdq_op_domain = ms_domain if extra_options.get("UseQDQContribOps", False) else None

        # The ONNX spec does not yet support 16-bit Q/DQ ops. So, must override the Q/DQ op domain to 'com.microsoft'
        # if the activation or weight types are 16-bit integers.
        # TODO: Remove this override (and use only the 'UseQDQContribOps' option) if/when ONNX adds 16-bit support.
        int16_types = (TensorProto.UINT16, TensorProto.INT16)
        overrides_have_int16 = any(t in int16_types for t in self.tensor_quant_override_types)
        if not self.qdq_op_domain and (
            self.activation_qType in int16_types or self.weight_qType in int16_types or overrides_have_int16
        ):
            logging.warning(
                "ONNX QuantizeLinear and DequantizeLinear operators do not support 16-bit integer quantization types. "
                f"The domain of QuantizeLinear and DequantizeLinear operators will be set to '{ms_domain}' to "
                "enable support."
            )
            self.qdq_op_domain = ms_domain

        self.quantization_params = self.calculate_graph_quantization_params()

        # Map of all original value names to quantized value names
        self.quantized_value_map = {}

    def _get_tensor_type(self, tensor_name):
        """
        Check if tensor can be quantized
        """
        weight = find_by_name(tensor_name, self.model.initializer())
        if weight is not None:
            return weight.data_type
        elif tensor_name in self.value_infos:
            vi = self.value_infos[tensor_name]
            if vi.type.HasField("tensor_type"):
                return vi.type.tensor_type.elem_type
        return None

    def _is_tensor_quantizable(self, tensor_name):
        """
        Check if tensor can be quantized
        """
        weight = find_by_name(tensor_name, self.model.initializer())
        if weight is not None:
            if weight.data_type in (onnx_proto.TensorProto.FLOAT, onnx_proto.TensorProto.FLOAT16):
                return True
        elif tensor_name in self.value_infos:
            vi = self.value_infos[tensor_name]
            if vi.type.HasField("tensor_type") and vi.type.tensor_type.elem_type in (
                TensorProto.FLOAT,
                TensorProto.FLOAT16,
            ):
                return True
        else:
            logging.warning(
                "failed to infer the type of tensor: {}. Skip to quantize it. Please check if it is expected.".format(
                    tensor_name
                )
            )

        return False

    def __quantize_tensor(self, tensor_name, quant_sharing_provider=None, tensor_type=QDQQuantTensorType.ACTIVATION):
        """
        Quantize tensors. If quant_param_tensor is not None, tensor with name tensor_name will be quantized with same
        quantization parameters as tensor quant_param_tensor

        Args:
            tensor_name: name of the tensor to quantize
            quant_sharing_provider: name of the tensor and node that provides quantization parameter
            tensor_type: QDQQuantTensorType default ACTIVATION
        """
        if self._is_tensor_quantizable(tensor_name):
            if quant_sharing_provider:
                if not isinstance(quant_sharing_provider, QDQQuantParamProvider):
                    raise TypeError(
                        f"quant_sharing_provider must be of type QDQQuantParamProvider, not {type(quant_sharing_provider)}."
                    )

                data_type = self._get_tensor_type(tensor_name)
                self.tensors_to_quantize[tensor_name] = QDQTensorQuantInfo(
                    tensor_type=tensor_type, quant_para_provider=quant_sharing_provider, data_type=data_type
                )
            elif tensor_name not in self.tensors_to_quantize:
                data_type = self._get_tensor_type(tensor_name)
                self.tensors_to_quantize[tensor_name] = QDQTensorQuantInfo(tensor_type=tensor_type, data_type=data_type)

    def quantize_activation_tensor(self, tensor_name):
        """
        Quantize Activation Tensor
        Args:
            tensor_name: name of the tensor to quantize
        """
        return self.__quantize_tensor(tensor_name, None, QDQQuantTensorType.ACTIVATION)

    def quantize_output_same_as_input(self, output_name, input_name, node_name):
        """
        Quantize a node's output the same as one of its inputs.
        """
        return self.__quantize_tensor(
            output_name, QDQQuantParamProvider(input_name, node_name), QDQQuantTensorType.ACTIVATION
        )

    def quantize_weight_tensor(self, tensor_name):
        """
        Quantize Weight Tensor
        Args:
            tensor_name: name of the tensor to quantize

        """
        return self.__quantize_tensor(tensor_name, None, QDQQuantTensorType.WEIGHT)

    def quantize_weight_tensor_per_channel(self, tensor_name, axis):
        weight = find_by_name(tensor_name, self.model.initializer())
        if weight:
            if weight.data_type in (onnx_proto.TensorProto.FLOAT, onnx_proto.TensorProto.FLOAT16):
                self.tensors_to_quantize[tensor_name] = QDQTensorQuantInfo(
                    tensor_type=QDQQuantTensorType.WEIGHT, axis=axis, data_type=weight.data_type
                )
        else:
            logging.warning(f"only support per-channel quantization on weight. Tensor: {tensor_name} is not quantized.")

    def quantize_bias_tensor(self, node_name, bias_name, input_name, weight_name, beta=1.0):
        # If the user provided quantization overrides for this tensor, treat it as a regular weight.
        if self.tensor_quant_overrides.get(bias_name):
            logging.info(
                f"Quantizing bias tensor '{bias_name}' as a weight due to the presence of user-specified overrides"
            )
            if self.per_channel:
                self.quantize_weight_tensor_per_channel(bias_name, 0)
            else:
                self.quantize_weight_tensor(bias_name)
            return

        weight = find_by_name(bias_name, self.model.initializer())
        if weight is not None:
            if weight.data_type in (onnx_proto.TensorProto.FLOAT, onnx_proto.TensorProto.FLOAT16):
                self.bias_to_quantize.append((node_name, bias_name, input_name, weight_name, beta))
        else:
            logging.warning(f"Expected {bias_name} to be a weight")

    def remove_node(self, node):
        self.nodes_to_remove.append(node)

    def remove_nodes(self):
        self.model.remove_nodes(self.nodes_to_remove)

    def quantize_model(self):
        for node in self.model.nodes():
            if self.should_quantize_node(node):
                op_quantizer = CreateQDQQuantizer(self, node)
                op_quantizer.quantize()

                for tensor_name in node.input:
                    if tensor_name not in self.tensor_to_its_receiving_nodes:
                        self.tensor_to_its_receiving_nodes[tensor_name] = []
                    self.tensor_to_its_receiving_nodes[tensor_name].append(node)

        self._quantize_normal_tensors()
        self._quantize_sharing_param_tensors()
        if self.quantize_bias:
            self._quantize_bias_tensors()
        self.remove_nodes()
        if not self.add_qdq_pair_to_weight:
            self.model.clean_initializers()

        self.model.model.producer_name = __producer__
        self.model.model.producer_version = __version__
        if self.qdq_op_domain == ms_domain:
            self.model.set_opset_import(ms_domain, 1)

        return self.model.model

    def try_replacing_upstream_output(self, upstream_output_name, output_name):
        if (
            output_name in self.quantization_params
            and len(self.model.input_name_to_nodes()[upstream_output_name]) == 1
            and not self.model.is_graph_output(upstream_output_name)
            and not self.model.is_graph_input(upstream_output_name)
        ):
            # TODO: Check same qtype ??
            self.model.replace_output_of_all_nodes(upstream_output_name, output_name)
            if upstream_output_name in self.tensors_to_quantize:
                del self.tensors_to_quantize[upstream_output_name]
            return True
        return False

    def _create_q_node(self, q_input, q_output, quant_node_name, scale_name, zp_name, axis=None):
        qlinear_node = onnx.helper.make_node(
            QUANT_OP_NAME,
            [q_input, scale_name, zp_name],
            [q_output],
            quant_node_name,
            axis=axis,
            domain=self.qdq_op_domain,
        )
        self.model.add_nodes([qlinear_node])

    def _create_dq_node(self, dq_input, dq_output, dequant_node_name, scale_name, zp_name, axis=None):
        dequant_node = onnx.helper.make_node(
            DEQUANT_OP_NAME,
            [dq_input, scale_name, zp_name],
            [dq_output],
            dequant_node_name,
            axis=axis,
            domain=self.qdq_op_domain,
        )
        self.model.add_nodes([dequant_node])

    def _create_qdq_nodes(
        self, q_input, q_output, quant_node_name, dq_input, dq_output, dequant_node_name, scale_name, zp_name, axis=None
    ):
        qlinear_node = onnx.helper.make_node(
            QUANT_OP_NAME,
            [q_input, scale_name, zp_name],
            [q_output],
            quant_node_name,
            axis=axis,
            domain=self.qdq_op_domain,
        )
        dequant_node = onnx.helper.make_node(
            DEQUANT_OP_NAME,
            [dq_input, scale_name, zp_name],
            [dq_output],
            dequant_node_name,
            axis=axis,
            domain=self.qdq_op_domain,
        )
        self.model.add_nodes([qlinear_node, dequant_node])

    def _add_qdq_pair_for_initializer(self, weight_proto, tensor_type, axis=None):
        weight_name = weight_proto.name
        if axis is not None:
            if self.opset_version < 13:
                raise ValueError("Per-Channel support with QDQ format requires onnx opset version 13 or above.")
            qtype = self.activation_qType
            if self.activation_qType == onnx.onnx_pb.TensorProto.UINT8:
                qtype = onnx_proto.TensorProto.INT8
            q_weight_name, zp_name, scale_name = self.quantize_weight_per_channel(
                weight_name,
                # Quantization type is forced to be TensorProto.INT8.
                # when the expected value would be (see below)
                # self.weight_qType if tensor_type is QDQQuantTensorType.WEIGHT else self.activation_qType.
                # QLinearConv expects to have a unique value for all channels.
                # This code does not enforce that but it is necessarily the case when the
                # quantization is symmetric (as for INT8).
                qtype,
                axis,
                keep_float_weight=self.add_qdq_pair_to_weight,
            )
        else:
            q_weight_name, zp_name, scale_name = self.quantize_initializer(
                weight_proto,
                self.weight_qType if tensor_type is QDQQuantTensorType.WEIGHT else self.activation_qType,
                keep_float_weight=self.add_qdq_pair_to_weight,
            )

        weight_dequant_output = add_dequant_output_suffix(weight_name)
        self.model.replace_input_of_all_nodes(weight_name, weight_dequant_output)
        if self.add_qdq_pair_to_weight:
            weight_quant_output = add_quant_output_suffix(weight_name)

            self._create_qdq_nodes(
                weight_name,
                weight_quant_output,
                add_quant_suffix(weight_name),
                weight_quant_output,
                weight_dequant_output,
                add_dequant_suffix(weight_name),
                scale_name,
                zp_name,
                axis,
            )
        else:
            dequant_node = onnx.helper.make_node(
                DEQUANT_OP_NAME,
                [q_weight_name, scale_name, zp_name],
                [weight_dequant_output],
                add_dequant_suffix(weight_name),
                axis=axis,
                domain=self.qdq_op_domain,
            )
            self.model.add_node(dequant_node)

    def _add_qdq_pair_for_activation(self, tensor_name, scale_name, zp_name, data_type=None):
        if (
            self.dedicated_qdq_pair
            and tensor_name in self.tensor_to_its_receiving_nodes
            and len(self.tensor_to_its_receiving_nodes[tensor_name]) > 1
        ):
            num_dedicated_qdq_pair = len(self.tensor_to_its_receiving_nodes[tensor_name])
            for i in range(num_dedicated_qdq_pair):
                postfix = f"_{i + 1}"
                tensor_name_quant_output_postfix = add_quant_output_suffix(tensor_name) + postfix
                tensor_name_dequant_output_postfix = add_dequant_output_suffix(tensor_name) + postfix
                quant_node_name_postfix = add_quant_suffix(tensor_name) + postfix
                dequant_node_name_postfix = add_dequant_suffix(tensor_name) + postfix
                self._create_qdq_nodes(
                    tensor_name,
                    tensor_name_quant_output_postfix,
                    quant_node_name_postfix,
                    tensor_name_quant_output_postfix,
                    tensor_name_dequant_output_postfix,
                    dequant_node_name_postfix,
                    scale_name,
                    zp_name,
                )

                node = self.tensor_to_its_receiving_nodes[tensor_name][i]
                self.model.replace_node_input(node, tensor_name, tensor_name_dequant_output_postfix)
                if i == 0:
                    quantized_value = QuantizedValue(
                        tensor_name,
                        tensor_name_dequant_output_postfix,
                        scale_name,
                        zp_name,
                        QuantizedValueType.Input,
                        scale_type=data_type,
                    )
                    self.quantized_value_map[tensor_name] = QDQTensorQuantizedValue(quantized_value, None, None)
        else:
            q_input = tensor_name
            dq_output = add_dequant_output_suffix(tensor_name)
            if self.model.is_graph_output(tensor_name):
                q_input = add_quant_input_suffix(tensor_name)
                dq_output = tensor_name
                self.model.replace_output_of_all_nodes(tensor_name, q_input)
            else:
                self.model.replace_input_of_all_nodes(tensor_name, dq_output)

            self._create_qdq_nodes(
                q_input,
                add_quant_output_suffix(tensor_name),
                add_quant_suffix(tensor_name),
                add_quant_output_suffix(tensor_name),
                dq_output,
                add_dequant_suffix(tensor_name),
                scale_name,
                zp_name,
            )

            quantized_value = QuantizedValue(
                tensor_name,
                dq_output,
                scale_name,
                zp_name,
                QuantizedValueType.Input,
                scale_type=data_type,
            )
            self.quantized_value_map[tensor_name] = QDQTensorQuantizedValue(quantized_value, None, None)

    def _add_qdq_ops_for_converted_activation(
        self,
        tensor_name,
        first_scale_name,
        first_zp_name,
        scale_data_type,
        convert_scale_name,
        convert_zp_name,
        convert_recv_nodes,
    ):
        tensor_recv_nodes = set([node.name for node in self.tensor_to_its_receiving_nodes[tensor_name]])

        if (
            self.dedicated_qdq_pair
            and tensor_name in self.tensor_to_its_receiving_nodes
            and len(self.tensor_to_its_receiving_nodes[tensor_name]) > 1
        ):
            raise ValueError(
                "Do not currently support converted quant_types in TensorQuantOverrides when the `dedicated_qdq_pair` extra_option is enabled"
            )
            return

        # Determine which nodes consume the original quantized type and which nodes
        # consume the converted quantized type.
        original_recv_nodes = tensor_recv_nodes
        if convert_recv_nodes is None:  # In this case, all consumers receive the converted type.
            convert_recv_nodes = tensor_recv_nodes
            original_recv_nodes = set()
        else:
            original_recv_nodes = original_recv_nodes - convert_recv_nodes

        all_use_converted = len(convert_recv_nodes) == len(tensor_recv_nodes)
        is_graph_output = self.model.is_graph_output(tensor_name)

        # Create first Q op.
        first_q_input = tensor_name
        if is_graph_output:
            first_q_input = add_quant_input_suffix(tensor_name)
            self.model.replace_output_of_all_nodes(tensor_name, first_q_input)

        first_q_output = add_quant_output_suffix(tensor_name)
        self._create_q_node(
            first_q_input, first_q_output, add_quant_suffix(tensor_name), first_scale_name, first_zp_name
        )

        # Create first DQ op.
        first_dq_output = add_dequant_output_suffix(tensor_name)
        if is_graph_output and not all_use_converted:
            first_dq_output = tensor_name
        if original_recv_nodes and first_dq_output != tensor_name:
            self.model.replace_input_of_nodes(tensor_name, first_dq_output, original_recv_nodes)

        self._create_dq_node(
            first_q_output, first_dq_output, add_dequant_suffix(tensor_name), first_scale_name, first_zp_name
        )

        # Create parallel clone of first DQ op if some nodes use the original quantized type.
        # This DQ clone would only have one consumer Q node and could be potentially fused with
        # it (the Q op) without breaking "node units".
        second_q_input = first_dq_output
        if not all_use_converted:
            second_q_input = add_quant_input_suffix(f"{tensor_name}_convert")
            self._create_dq_node(
                first_q_output,
                second_q_input,
                add_dequant_suffix(f"{tensor_name}_convert_clone"),
                first_scale_name,
                first_zp_name,
            )

        # Create second Q op.
        second_q_output = add_quant_output_suffix(f"{tensor_name}_convert")
        self._create_q_node(
            second_q_input,
            second_q_output,
            add_quant_suffix(f"{tensor_name}_convert"),
            convert_scale_name,
            convert_zp_name,
        )

        # Create second DQ op.
        second_dq_output = add_dequant_output_suffix(f"{tensor_name}_convert")
        if is_graph_output and all_use_converted:
            second_dq_output = tensor_name
        if convert_recv_nodes and second_dq_output != tensor_name:
            self.model.replace_input_of_nodes(tensor_name, second_dq_output, convert_recv_nodes)
        self._create_dq_node(
            second_q_output,
            second_dq_output,
            add_dequant_suffix(f"{tensor_name}_convert"),
            convert_scale_name,
            convert_zp_name,
        )

        # Store in quantized_value_map
        original_quantized_value = QuantizedValue(
            tensor_name,
            first_dq_output,
            first_scale_name,
            first_zp_name,
            QuantizedValueType.Input,
            scale_type=scale_data_type,
        )
        converted_quantized_value = QuantizedValue(
            tensor_name,
            second_dq_output,
            convert_scale_name,
            convert_zp_name,
            QuantizedValueType.Input,
            scale_type=scale_data_type,
        )
        self.quantized_value_map[tensor_name] = QDQTensorQuantizedValue(
            original_quantized_value, converted_quantized_value, convert_recv_nodes
        )

    def _quantize_normal_tensors(self):
        for tensor_name, tensor_info in self.tensors_to_quantize.copy().items():
            if tensor_name in self.quantized_value_map:
                continue

            if not tensor_info.is_shared:
                # Quantize the input
                initializer = find_by_name(tensor_name, self.model.initializer())
                if initializer:
                    self._add_qdq_pair_for_initializer(initializer, tensor_info.tensor_type, tensor_info.axis)
                else:
                    quant_params_initializers = self._create_quantization_initializers(tensor_name)
                    if not quant_params_initializers:
                        raise ValueError(
                            f"Quantization parameters are not specified for param {tensor_name}. "
                            "In static mode quantization params for inputs and outputs of nodes to be quantized are required."
                        )

                    if quant_params_initializers.converted is None:
                        self._add_qdq_pair_for_activation(
                            tensor_name,
                            quant_params_initializers.original.scale.name,
                            quant_params_initializers.original.zero_point.name,
                            data_type=tensor_info.data_type,
                        )
                    else:
                        assert tensor_info.data_type == quant_params_initializers.original.scale.data_type
                        self._add_qdq_ops_for_converted_activation(
                            tensor_name,
                            quant_params_initializers.original.scale.name,
                            quant_params_initializers.original.zero_point.name,
                            tensor_info.data_type,
                            quant_params_initializers.converted.scale.name,
                            quant_params_initializers.converted.zero_point.name,
                            quant_params_initializers.converted_recv_nodes,
                        )

                del self.tensors_to_quantize[tensor_name]

    def _quantize_sharing_param_tensors(self):
        while self.tensors_to_quantize:
            for tensor_name, tensor_info in self.tensors_to_quantize.copy().items():
                quant_provider = tensor_info.quant_para_provider
                if quant_provider and quant_provider.input_name in self.quantized_value_map:
                    del self.tensors_to_quantize[tensor_name]

                    quantized_value = self.quantized_value_map[quant_provider.input_name].get_for_consumer(
                        quant_provider.node_name
                    )
                    if self.is_input_a_initializer(tensor_name):
                        raise ValueError("Quantization parameter shared mode is not supported for weight yet")

                    # Don't overlook potential conversion at the output.
                    converted_qparam_inits = None
                    converted_recv_nodes = None
                    if tensor_name in self.quantization_params:
                        tensor_params = self.quantization_params[tensor_name]
                        if tensor_params.converted:
                            converted_qparam_inits = self.__make_initializers(
                                tensor_name, tensor_params.converted, "_convert"
                            )
                            converted_recv_nodes = tensor_params.converted_recv_nodes

                    if converted_qparam_inits is not None:
                        self._add_qdq_ops_for_converted_activation(
                            tensor_name,
                            quantized_value.scale_name,
                            quantized_value.zp_name,
                            converted_qparam_inits.scale.data_type,
                            converted_qparam_inits.scale.name,
                            converted_qparam_inits.zero_point.name,
                            converted_recv_nodes,
                        )
                    else:
                        self._add_qdq_pair_for_activation(
                            tensor_name, quantized_value.scale_name, quantized_value.zp_name
                        )

    def _quantize_bias_tensors(self):
        for bias_node_name, bias_name, input_name, weight_name, beta in self.bias_to_quantize:
            if bias_name in self.quantized_value_map:
                continue
            # Quantize the input
            self.quantize_bias_static(bias_node_name, bias_name, input_name, weight_name, beta)
            init = find_by_name(bias_name, self.model.initializer())
            self.model.remove_initializer(init)
            quant_value = self.quantized_value_map[bias_name].original
            if quant_value.node_type == "Cast":
                # simple cast to float 16 and not DequantizeLinear
                # cublasLtMatmul only supports (b)float16, float bias.
                if not isinstance(init.data_type, int):
                    raise TypeError(f"Unexpected type {type(init.data_type)} for input={input_name!r}")
                node_name = add_dequant_suffix(bias_name)
                dequant_node = onnx.helper.make_node(
                    "Cast",
                    [quant_value.q_name],
                    [bias_name],
                    name=node_name,
                    to=init.data_type,
                )
            elif quant_value.node_type in (None, "DequantizeLinear"):
                if quant_value.node_qtype in {
                    onnx.TensorProto.FLOAT16,
                    onnx.TensorProto.BFLOAT16,
                    onnx.TensorProto.FLOAT,
                }:
                    raise RuntimeError(f"Unexpected quantize type {quant_value.node_qtype} for DequantizeLinear.")
                inputs = [quant_value.q_name, quant_value.scale_name, quant_value.zp_name]
                node_name = add_dequant_suffix(bias_name)
                if quant_value.axis is not None:
                    dequant_node = onnx.helper.make_node(
                        "DequantizeLinear",
                        inputs,
                        [bias_name],
                        node_name,
                        axis=quant_value.axis,
                        domain=self.qdq_op_domain,
                    )
                else:
                    dequant_node = onnx.helper.make_node(
                        "DequantizeLinear",
                        inputs,
                        [bias_name],
                        node_name,
                        domain=self.qdq_op_domain,
                    )
            else:
                raise RuntimeError(f"Unexpected operator type {quant_value.node_type!r}.")
            self.model.add_node(dequant_node)

    def is_tensor_quantized(self, tensor_name):
        return tensor_name in self.tensors_to_quantize

    def quantize_initializer(self, weight, qType, reduce_range=False, keep_float_weight=False):
        """
        :param weight: TensorProto initializer
        :param qType: type to quantize to
        :param keep_float_weight: Whether to quantize the weight. In some cases, we only want to qunatize scale and zero point.
                                  If keep_float_weight is False, quantize the weight, or don't quantize the weight.
        :return: quantized weight name, zero point name, scale name
        """
        # Find if this input is already quantized
        if weight.name in self.quantized_value_map:
            quantized_value = self.quantized_value_map[weight.name].original
            return (
                quantized_value.q_name,
                quantized_value.zp_name,
                quantized_value.scale_name,
            )

        q_weight_name, zp_name, scale_name = self.quantize_initializer_impl(
            weight, qType, reduce_range, keep_float_weight
        )

        # Log entry for this quantized weight
        quantized_value = QuantizedValue(
            weight.name,
            q_weight_name,
            scale_name,
            zp_name,
            QuantizedValueType.Initializer,
            None,
        )
        self.quantized_value_map[weight.name] = QDQTensorQuantizedValue(quantized_value, None, None)
        return q_weight_name, zp_name, scale_name

    def quantize_weight_per_channel(
        self,
        weight_name,
        weight_qType,
        channel_axis,
        reduce_range=True,
        keep_float_weight=False,
    ):
        # Find if this input is already quantized
        if weight_name in self.quantized_value_map:
            quantized_value = self.quantized_value_map[weight_name].original
            return (
                quantized_value.q_name,
                quantized_value.zp_name,
                quantized_value.scale_name,
            )

        q_weight_name, zp_name, scale_name = self.quantize_weight_per_channel_impl(
            weight_name, weight_qType, channel_axis, reduce_range, keep_float_weight
        )
        quantized_value = QuantizedValue(
            weight_name,
            q_weight_name,
            scale_name,
            zp_name,
            QuantizedValueType.Initializer,
            None,
        )
        self.quantized_value_map[weight_name] = QDQTensorQuantizedValue(quantized_value, None, None)

        return q_weight_name, zp_name, scale_name

    def quantize_bias_static(self, node_name, bias_name, input_name, weight_name, beta=1.0):
        """
        Quantized the bias. Zero Point == 0 and Scale == Input_Scale * Weight_Scale
        """

        # Handle case where bias already in quantization map
        if bias_name in self.quantized_value_map:
            return self.quantized_value_map[bias_name].original.q_name

        # get scale for weight
        weight_scale_name = self.quantized_value_map[weight_name].original.scale_name
        weight_initializer = find_by_name(weight_scale_name, self.model.initializer())
        weight_scale = tensor_proto_to_array(weight_initializer)

        # get scale for input
        input_scale_name = self.quantized_value_map[input_name].get_for_consumer(node_name).scale_name
        inputscale_initializer = find_by_name(input_scale_name, self.model.initializer())
        input_scale = tensor_proto_to_array(inputscale_initializer)

        (
            quantized_bias_name,
            quantized_bias_scale_name,
            quantized_bias_zp_name,
            bias_scale_data,
            node_type,
            node_qtype,
        ) = self.quantize_bias_static_impl(bias_name, input_scale, weight_scale, beta)

        quantized_value = QuantizedValue(
            bias_name,
            quantized_bias_name,
            quantized_bias_scale_name,
            quantized_bias_zp_name,
            QuantizedValueType.Initializer,
            0 if bias_scale_data.size > 1 else None,
            node_type=node_type,
            node_qtype=node_qtype,
        )
        self.quantized_value_map[bias_name] = QDQTensorQuantizedValue(quantized_value, None, None)

        return quantized_bias_name

    def __make_initializers(
        self, param_name: str, params: QuantizationParams, init_name_suffix: str = ""
    ) -> QDQQuantParamsInitializers:
        zero_point_values = np.array([params["zero_point"]])
        if not hasattr(params["scale"], "dtype") or params["scale"].dtype not in (np.float32, np.float16):
            raise ValueError(f"Unexpected type {type(params['scale'])} and param_name={param_name!r}")
        scale_values = np.array([params["scale"]])
        assert scale_values.dtype != np.float64
        zero_point_type = params.data.get("quant_type", self.activation_qType)

        zero_point_shape = []
        zero_point_name = param_name + "_zero_point" + init_name_suffix
        scale_shape = []
        scale_name = param_name + "_scale" + init_name_suffix

        # Add initializers to model
        init_zp = onnx.helper.make_tensor(
            zero_point_name, zero_point_type, zero_point_shape, zero_point_values.ravel().tolist()
        )
        self.model.add_initializer(init_zp)

        if scale_values.dtype == np.float32:
            scale_type = onnx_proto.TensorProto.FLOAT
        elif scale_values.dtype == np.float16:
            scale_type = onnx_proto.TensorProto.FLOAT16
        else:
            raise ValueError(f"Unexpected dtype={scale_values.dtype} for param_name={param_name!r}")
        init_scale = onnx.helper.make_tensor(scale_name, scale_type, scale_shape, scale_values.reshape((-1,)).tolist())
        self.model.add_initializer(init_scale)

        return QDQQuantParamsInitializers(init_scale, init_zp)

    def _create_quantization_initializers(self, param_name: str) -> QDQTensorQuantParamsInitializers | None:
        """
        Create initializers and inputs in the graph for zero point and scale of output.
        Zero point and scale values are obtained from self.quantization_params.
            parameter param_name: Name of the quantization parameter.
            return: QDQTensorQuantParamsInitializers
        """
        if self.quantization_params is None or param_name not in self.quantization_params:
            logging.info(f'Quantization parameters for tensor:"{param_name}" not specified')
            return None

        tensor_params = self.quantization_params[param_name]
        if not isinstance(tensor_params, QDQTensorQuantParams):
            raise TypeError(f"Unexpected type {type(tensor_params)} for {param_name!r}.")

        original_inits = self.__make_initializers(param_name, tensor_params.original)
        converted_inits = (
            self.__make_initializers(param_name, tensor_params.converted, "_convert")
            if tensor_params.converted
            else None
        )

        return QDQTensorQuantParamsInitializers(original_inits, converted_inits, tensor_params.converted_recv_nodes)

    def calculate_quantization_params(self, tensor_data: TensorData, quant_overrides) -> QuantizationParams:
        quant_type = self.activation_qType
        if "quant_type" in quant_overrides:
            quant_type = quant_overrides["quant_type"].tensor_type

        if "scale" in quant_overrides and "zero_point" in quant_overrides:
            zero, scale = quant_overrides["zero_point"], quant_overrides["scale"]
        elif quant_type == onnx.TensorProto.FLOAT8E4M3FN:
            zero, scale = compute_scale_zp_float8(quant_type, tensor_data.avg_std[1])
        else:
            rmin = quant_overrides.get("rmin", tensor_data.range_value[0])
            rmax = quant_overrides.get("rmax", tensor_data.range_value[1])
            symmetric = quant_overrides.get("symmetric", self.is_activation_symmetric)
            reduce_range = quant_overrides.get("reduce_range", False)
            qmin, qmax = get_qmin_qmax_for_qType(quant_type, reduce_range=reduce_range, symmetric=symmetric)
            zero, scale = compute_scale_zp(rmin, rmax, qmin, qmax, symmetric, self.min_real_range)

        return QuantizationParams(zero_point=zero, scale=scale, quant_type=quant_type)

    def calculate_graph_quantization_params(self):
        if self.tensors_range is None:
            return

        self.adjust_tensor_ranges(softmax_0_to_1=True)

        quantization_params = {}
        for tensor_name in self.tensors_range:
            td = self.tensors_range[tensor_name]
            if not isinstance(td, TensorData):
                raise TypeError(f"Unexpected type {type(td)} for {tensor_name!r}.")

            quant_overrides = self.get_per_tensor_quant_overrides(tensor_name)
            original = self.calculate_quantization_params(td, quant_overrides)
            converted = None
            converted_recv_nodes = None

            if "convert" in quant_overrides:
                converted = self.calculate_quantization_params(td, quant_overrides["convert"])
                converted_recv_nodes = quant_overrides["convert"].get("recv_nodes")

            quantization_params[tensor_name] = QDQTensorQuantParams(original, converted, converted_recv_nodes)

        return quantization_params
