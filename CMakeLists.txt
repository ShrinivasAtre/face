cmake_minimum_required(VERSION 3.10)

project(YuNetFaceDetection LANGUAGES CXX)

include(CTest)

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
    src/LbfLandmarkDetector.cpp
    src/LbfEyeLandmarkMapper.cpp
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
# Models are copied next to the executable:
#
# Windows:
#   build/Release/models/
#
# Linux / Orin:
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
#
# The DLL path is supplied when configuring CMake:
#
# -DOPENCV_DLL="C:/.../opencv_world480.dll"
#
# This keeps machine-specific paths out of the repository.
# ============================================================

if(WIN32)

    set(OPENCV_DLL
        ""
        CACHE FILEPATH
        "Path to OpenCV Windows DLL"
    )


    if(OPENCV_DLL STREQUAL "")

        message(FATAL_ERROR
            "OPENCV_DLL is not set. "
            "Configure with "
            "-DOPENCV_DLL=<path-to-opencv_world480.dll>"
        )

    endif()


    if(NOT EXISTS "${OPENCV_DLL}")

        message(FATAL_ERROR
            "OpenCV DLL does not exist: ${OPENCV_DLL}"
        )

    endif()


    message(STATUS
        "OpenCV DLL: ${OPENCV_DLL}"
    )


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


# ============================================================
# Focused blink-processing tests
# ============================================================

if(BUILD_TESTING)

    set(FACE_TEST_IMAGE
        ""
        CACHE FILEPATH
        "Optional face image used by the legacy blink integration test"
    )

    add_executable(blink_tracker_test
        tests/blink_tracker_test.cpp
        src/BlinkTracker.cpp
    )

    target_include_directories(blink_tracker_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(blink_tracker_test
        PRIVATE
            ${OpenCV_LIBS}
    )

    add_test(
        NAME blink_tracker_test
        COMMAND blink_tracker_test
    )

    add_executable(lbf_eye_landmark_mapper_test
        tests/lbf_eye_landmark_mapper_test.cpp
        src/LbfEyeLandmarkMapper.cpp
    )

    target_include_directories(lbf_eye_landmark_mapper_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(lbf_eye_landmark_mapper_test
        PRIVATE
            ${OpenCV_LIBS}
    )

    add_test(
        NAME lbf_eye_landmark_mapper_test
        COMMAND lbf_eye_landmark_mapper_test
    )

    add_executable(legacy_blink_integration_test
        tests/legacy_blink_integration_test.cpp
        src/FaceDetector.cpp
        src/LbfLandmarkDetector.cpp
        src/LbfEyeLandmarkMapper.cpp
        src/BlinkTracker.cpp
    )

    target_include_directories(legacy_blink_integration_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(legacy_blink_integration_test
        PRIVATE
            ${OpenCV_LIBS}
    )

    if(NOT FACE_TEST_IMAGE STREQUAL "")

        if(NOT EXISTS "${FACE_TEST_IMAGE}")
            message(FATAL_ERROR
                "FACE_TEST_IMAGE does not exist: ${FACE_TEST_IMAGE}"
            )
        endif()

        add_test(
            NAME legacy_blink_integration_test
            COMMAND legacy_blink_integration_test
                "${CMAKE_CURRENT_SOURCE_DIR}/models/face_detection_yunet_2026may.onnx"
                "${CMAKE_CURRENT_SOURCE_DIR}/models/lbfmodel.yaml"
                "${FACE_TEST_IMAGE}"
        )

    endif()

endif()
