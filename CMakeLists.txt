cmake_minimum_required(VERSION 3.10)

project(YuNetFaceDetection LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Show which OpenCV CMake found
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

# Application
add_executable(yunet_demo
    main.cpp
)

# OpenCV include directories
target_include_directories(yunet_demo
    PRIVATE
        ${OpenCV_INCLUDE_DIRS}
)

# OpenCV libraries
target_link_libraries(yunet_demo
    PRIVATE
        ${OpenCV_LIBS}
)