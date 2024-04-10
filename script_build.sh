
rm -rf build

# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda --skip_submodule_sync --build_wheel
# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.6.0.12/ --skip_submodule_sync --build_wheel

./build.sh --enable_pybind --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.4.3.1 --skip_submodule_sync --build_wheel --skip_tests --skip_onnx_tests
# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.4.3.1 --skip_submodule_sync --build_wheel

# python -m pip install --force-reinstall ./build/Linux/Debug/dist/onnxruntime_gpu-1.16.2-cp37-cp37m-linux_x86_64.whl
 # sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 23

###########
# TVM x86
###########
# ./build.sh --config Debug --enable_pybind --build_wheel --parallel --skip_tests --skip_onnx_tests --use_tvm 
# ./build.sh --config Debug --enable_pybind --build_wheel --parallel --skip_tests --skip_onnx_tests --use_tvm --tvm_cuda_runtime
# ./build.sh --config Debug --enable_pybind --build_wheel --parallel --skip_tests --skip_onnx_tests --use_tvm --tvm_cuda_runtime --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda
# ./build.sh --config Debug --enable_pybind --build_wheel --parallel --skip_tests --skip_onnx_tests --use_tvm --tvm_cuda_runtime --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.4.3.1 --skip_submodule_sync

# python -m pip uninstall onnxruntime-tvm
# python -m pip uninstall tvm -y
# python -m pip install --force-reinstall ./build/Linux/Debug/_deps/tvm-src/python/dist/tvm-0.14.0-cp37-cp37m-linux_x86_64.whl
# python -m pip install --force-reinstall ./build/Linux/Debug/dist/onnxruntime_tvm-1.16.2-cp37-cp37m-linux_x86_64.whl

