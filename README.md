# face
Face recognition

For Windows 

cmake -S . -B build -DOpenCV_DIR="C:\opencv-4.8.0-src\install\lib"
cmake --build build --config Release



For Orin
cmake -S . -B build -DOpenCV_DIR=/usr/local/opencv-4.8.0-contrib/lib/cmake/opencv4
 cmake --build build -j4



New -----
Windows 
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue

cmake -S . -B build `
  -DOpenCV_DIR="C:\opencv-4.8.0-src\install\lib" `
  -DOPENCV_DLL="C:\opencv-4.8.0-src\install\bin\opencv_world480.dll"
  
cmake --build build --config Release


Orin
