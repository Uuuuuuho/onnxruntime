# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda --skip_submodule_sync --build_wheel
# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.6.0.12/ --skip_submodule_sync --build_wheel

# ./build.sh --enable_pybind --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.4.3.1 --skip_submodule_sync --use_tvm --tvm_cuda_runtime --build_wheel
# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.4.3.1 --skip_submodule_sync --build_wheel

 # sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 23

###########
# TVM x86
###########
./build.sh --config Release --enable_pybind --build_wheel --parallel --skip_tests --skip_onnx_tests --use_tvm --llvm_config /home/fsir/workspace/build-llvm/bin/llvm-config 
# /home/fsir/Downloads/cmake-3.27.5-linux-x86_64/bin/cmake --build /home/fsir/workspace/onnxruntime/build/Linux/Release --config Release -- -j12
