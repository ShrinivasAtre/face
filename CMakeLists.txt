cmake_minimum_required(VERSION 3.10)

project(YuNetFaceDetection LANGUAGES CXX)

# ============================================================
# C++ standard
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ============================================================
# OpenCV
# ============================================================

find_package(OpenCV REQUIRED)

message(STATUS "----------------------------------------")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "----------------------------------------")


# ============================================================
# Application
# ============================================================

add_executable(yunet_demo

    src/main.cpp

    src/AppPaths.cpp

    src/FaceDetector.cpp

    src/BlinkTracker.cpp
)


# ============================================================
# Include directories
# ============================================================

target_include_directories(yunet_demo

    PRIVATE

        ${CMAKE_CURRENT_SOURCE_DIR}/include

        ${OpenCV_INCLUDE_DIRS}
)


# ============================================================
# Link OpenCV
# ============================================================

target_link_libraries(yunet_demo

    PRIVATE

        ${OpenCV_LIBS}
)


# ============================================================
# Model deployment
#
# The models are copied next to the executable.
#
# Windows:
#   build/Release/models/
#
# Linux / Jetson:
#   build/models/
# ============================================================

set(MODEL_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/models"
)


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


# ============================================================
# Windows OpenCV DLL deployment
# ============================================================

if(WIN32)

    # OpenCVConfig.cmake is located at:
    #
    # C:/opencv-4.8.0-src/install/lib/cmake/opencv4
    #
    # OpenCV DLL is located at:
    #
    # C:/opencv-4.8.0-src/install/bin/opencv_world480.dll
    #
    # We use CMAKE_PREFIX_PATH rather than OpenCV_DIR because
    # OpenCV_DIR can be normalized internally by the package.

    if(DEFINED CMAKE_PREFIX_PATH)

        list(GET CMAKE_PREFIX_PATH 0 OPENCV_PREFIX)

        set(OPENCV_DLL
            "${OPENCV_PREFIX}/bin/opencv_world480.dll"
        )

    endif()


    # If CMAKE_PREFIX_PATH was not supplied, use the known
    # OpenCV installation location as a fallback.

    if(NOT DEFINED OPENCV_DLL OR NOT EXISTS "${OPENCV_DLL}")

        set(OPENCV_DLL
            "C:/opencv-4.8.0-src/install/bin/opencv_world480.dll"
        )

    endif()


    message(STATUS
        "OpenCV DLL: ${OPENCV_DLL}"
    )


    if(NOT EXISTS "${OPENCV_DLL}")

        message(FATAL_ERROR
            "OpenCV DLL not found: ${OPENCV_DLL}"
        )

    endif()


    add_custom_command(
        TARGET yunet_demo
        POST_BUILD

        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            "${OPENCV_DLL}"
            "$<TARGET_FILE_DIR:yunet_demo>/opencv_world480.dll"

        COMMENT
            "Copying OpenCV DLL to executable directory"
    )

endif()