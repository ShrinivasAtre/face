cmake_minimum_required(VERSION 3.10)

project(YuNetFaceDetection LANGUAGES CXX)

# --------------------------------------------------
# C++ standard
# --------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# --------------------------------------------------
# OpenCV
# --------------------------------------------------

find_package(OpenCV REQUIRED)

message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")


# --------------------------------------------------
# Application
# --------------------------------------------------

add_executable(yunet_demo

    src/main.cpp

    src/AppPaths.cpp

    src/FaceDetector.cpp

    src/BlinkTracker.cpp
)


# --------------------------------------------------
# Include directories
# --------------------------------------------------

target_include_directories(yunet_demo

    PRIVATE

        ${CMAKE_CURRENT_SOURCE_DIR}/include

        ${OpenCV_INCLUDE_DIRS}
)


# --------------------------------------------------
# Link OpenCV
# --------------------------------------------------

target_link_libraries(yunet_demo

    PRIVATE

        ${OpenCV_LIBS}
)


# ==================================================
# Model deployment
# ==================================================

set(MODEL_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/models"
)


# Copy models beside executable
#
# Windows:
#   build/Release/models/
#
# Linux:
#   build/models/
#

add_custom_command(
    TARGET yunet_demo
    POST_BUILD

    COMMAND
        ${CMAKE_COMMAND}
        -E make_directory
        "$<TARGET_FILE_DIR:yunet_demo>/models"

    COMMAND
        ${CMAKE_COMMAND}
        -E copy_directory
        "${MODEL_SOURCE_DIR}"
        "$<TARGET_FILE_DIR:yunet_demo>/models"

    COMMENT
        "Copying model files to executable directory"
)