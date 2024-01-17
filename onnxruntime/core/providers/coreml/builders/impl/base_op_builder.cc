// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/coreml/builders/impl/base_op_builder.h"

#include "core/providers/common.h"
#include "core/providers/coreml/builders/helper.h"
#include "core/providers/shared/utils/utils.h"

// #ifdef __APPLE__OR__TEST__
#include "core/providers/coreml/builders/model_builder.h"
// #endif

namespace onnxruntime {
namespace coreml {

// Shared functions

// TODO, move this to shared_library
bool HasExternalInitializer(const InitializedTensorSet& initializers, const Node& node,
                            const logging::Logger& logger) {
  for (const auto* node_arg : node.InputDefs()) {
    const auto& input_name(node_arg->Name());
    const auto initializer_it = initializers.find(input_name);
    if (initializer_it == initializers.end()) {
      continue;
    }

    const auto& tensor = *initializer_it->second;
    if (tensor.has_data_location() &&
        tensor.data_location() == ONNX_NAMESPACE::TensorProto_DataLocation_EXTERNAL) {
      LOGS(logger, VERBOSE) << "Initializer [" << input_name
                            << "] with external data location are not currently supported";
      return true;
    }
  }

  return false;
}

// Add operator related
// #if defined(__APPLE__OR__TEST__) || defined(__linux__)
Status BaseOpBuilder::AddToModelBuilder(ModelBuilder& model_builder, const Node& node,
                                        const OpBuilderInputParams& input_params,
                                        const logging::Logger& logger) const {
  // TODO: This seems like an unnecessary duplicate call as it's only used for nodes in the EP Compile, which
  // should only ever be nodes we returned from GetCapability, and we called IsOpSupported there already.
  //
  // The only thing to potentially validate would be changes to the internal NHWC domain, but the preferred format
  // for the CoreML is the default NCHW layout so that is not a factor.
  ORT_RETURN_IF_NOT(IsOpSupported(node, input_params, logger),
                    "Unsupported operator ",
                    node.OpType());

  ORT_RETURN_IF_ERROR(AddToModelBuilderImpl(model_builder, node, logger));
  LOGS(logger, VERBOSE) << "Operator name: [" << node.Name() << "] type: [" << node.OpType() << "] was added";

  return Status::OK();
}

// #endif

// Operator support related

bool BaseOpBuilder::IsOpSupported(const Node& node, const OpBuilderInputParams& input_params,
                                  const logging::Logger& logger) const {
  if (!HasSupportedInputs(node, input_params, logger))
    return false;

  // We do not support external initializers for now
  const auto& initializers = input_params.graph_viewer.GetAllInitializedTensors();
  if (HasExternalInitializer(initializers, node, logger))
    return false;

  if (!HasSupportedOpSet(node, logger))
    return false;

  return IsOpSupportedImpl(node, input_params, logger);
}

bool BaseOpBuilder::HasSupportedInputs(const Node& node, const OpBuilderInputParams& input_params,
                                       const logging::Logger& logger) const {
  const auto node_name = MakeString("Node [", node.Name(), "] type [", node.OpType(), "]");
  for (const auto* input : node.InputDefs()) {
    if (!IsInputSupported(*input, node_name, input_params, logger)) {
      return false;
    }
  }

  return HasSupportedInputsImpl(node, logger);
}

/* static */
bool BaseOpBuilder::Input0IsSupported(const Node& node, const logging::Logger& logger) {
  const auto& input = *node.InputDefs()[0];

  int32_t input_type = ONNX_NAMESPACE::TensorProto_DataType_UNDEFINED;

  if (!GetType(input, input_type, logger) ||
      (input_type != ONNX_NAMESPACE::TensorProto_DataType_FLOAT)) {
    LOGS(logger, VERBOSE) << "[" << node.OpType() << "] Input type: [" << input_type << "] is not currently supported";
    return false;
  }

  return true;
}

bool BaseOpBuilder::HasSupportedInputsImpl(const Node& node, const logging::Logger& logger) const {
  // We only check the type of input 0 by default
  // specific op builder can override this
  return Input0IsSupported(node, logger);
}

bool BaseOpBuilder::HasSupportedOpSet(const Node& node, const logging::Logger& logger) const {
  auto since_version = node.SinceVersion();
  if (since_version < GetMinSupportedOpSet(node) || since_version > GetMaxSupportedOpSet(node)) {
    LOGS(logger, VERBOSE) << node.OpType() << "is only supported for opset ["
                          << GetMinSupportedOpSet(node) << ", "
                          << GetMaxSupportedOpSet(node) << "]";
    return false;
  }

  return true;
}

}  // namespace coreml
}  // namespace onnxruntime
