# face
Face recognition

Architecture: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

For Orin
cmake -S . -B build -DOpenCV_DIR=/usr/local/opencv-4.8.0-contrib/lib/cmake/opencv4

cmake --build build -j4

Windows 

Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

cmake -S . -B build -DOpenCV_DIR="C:\opencv-4.8.0-src\install\lib" -DOPENCV_DLL="C:\opencv-4.8.0-src\install\bin\opencv_world480.dll"
  
cmake --build build --config Release


cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DOpenCV_DIR=/usr/local/opencv-4.8.0-contrib/lib/cmake/opencv4 \
  -DFACE_ENABLE_MEDIAPIPE_RUNTIME=ON \
  -DFACE_MEDIAPIPE_PACKAGE_DIR=/home/shrinivas/common/p17/mediapipe-video-linux-aarch64

cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/yunet_demo --backend=mediapipe
