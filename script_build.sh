# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda --skip_submodule_sync --build_wheel
# ./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.6.0.12/ --skip_submodule_sync --build_wheel

./build.sh --parallel --use_cuda --cudnn_home=/usr/local/cuda --cuda_home=/usr/local/cuda  --use_tensorrt --tensorrt_home=/home/fsir/Downloads/TensorRT-8.4.3.1 --skip_submodule_sync --build_wheel

 # sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 23
