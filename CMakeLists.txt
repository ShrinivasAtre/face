cmake_minimum_required(VERSION 3.10)
project(YuNetFaceDetection)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the OpenCV library packages
find_package(OpenCV REQUIRED)

# Include OpenCV headers
include_directories(${OpenCV_INCLUDE_DIRS})

# Define the executable
add_executable(yunet_demo main.cpp)

# Link OpenCV libraries to the executable
target_link_libraries(yunet_demo ${OpenCV_LIBS})
