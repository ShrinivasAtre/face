cmake_minimum_required(VERSION 3.10)

project(YuNetFaceDetection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

add_executable(yunet_demo
    src/main.cpp
    src/FaceDetector.cpp
    src/BlinkTracker.cpp
    src/AppPaths.cpp
)

target_include_directories(yunet_demo
    PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(yunet_demo
    PRIVATE
        ${OpenCV_LIBS}
)

# Windows: copy OpenCV runtime DLL beside executable
if(WIN32)
    add_custom_command(TARGET yunet_demo POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/opencv-4.8.0-src/install/bin/opencv_world480.dll"
            "$<TARGET_FILE_DIR:yunet_demo>"
    )
endif()

# ------------------------------------------------------------
# Copy models beside executable
# ------------------------------------------------------------
add_custom_command(TARGET yunet_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:yunet_demo>/models"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/models/face_detection_yunet_2026may.onnx"
        "$<TARGET_FILE_DIR:yunet_demo>/models/face_detection_yunet_2026may.onnx"

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/models/lbfmodel.yaml"
        "$<TARGET_FILE_DIR:yunet_demo>/models/lbfmodel.yaml"
)