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
# Optional packaged MediaPipe runtime
#
# Bazel remains responsible for producing the package. CMake
# only validates and stages an explicitly supplied package.
# ============================================================

option(
    FACE_ENABLE_MEDIAPIPE_RUNTIME
    "Stage and enable the runtime-loaded FaceMediaPipe bridge"
    OFF
)

set(FACE_MEDIAPIPE_PACKAGE_DIR
    ""
    CACHE PATH
    "Path to a packaged FaceMediaPipe runtime directory"
)

if(FACE_ENABLE_MEDIAPIPE_RUNTIME)

    if(FACE_MEDIAPIPE_PACKAGE_DIR STREQUAL "")
        message(FATAL_ERROR
            "FACE_MEDIAPIPE_PACKAGE_DIR is required when "
            "FACE_ENABLE_MEDIAPIPE_RUNTIME=ON"
        )
    endif()

    get_filename_component(
        FACE_MEDIAPIPE_PACKAGE_DIR
        "${FACE_MEDIAPIPE_PACKAGE_DIR}"
        ABSOLUTE
    )

    if(WIN32)
        set(FACE_MEDIAPIPE_PACKAGE_PLATFORM "windows-x64")
        set(FACE_MEDIAPIPE_BRIDGE_NAME "FaceMediaPipe.dll")
        set(FACE_MEDIAPIPE_PACKAGE_OPENCV
            "${FACE_MEDIAPIPE_PACKAGE_DIR}/opencv_world480.dll"
        )
    elseif(
        CMAKE_SYSTEM_NAME STREQUAL "Linux" AND
        CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$"
    )
        set(FACE_MEDIAPIPE_PACKAGE_PLATFORM "linux-aarch64")
        set(FACE_MEDIAPIPE_BRIDGE_NAME "libFaceMediaPipe.so")
    else()
        message(FATAL_ERROR
            "Packaged MediaPipe runtime is supported only on "
            "Windows x64 and Linux aarch64"
        )
    endif()

    set(FACE_MEDIAPIPE_PACKAGE_BRIDGE
        "${FACE_MEDIAPIPE_PACKAGE_DIR}/${FACE_MEDIAPIPE_BRIDGE_NAME}"
    )
    set(FACE_MEDIAPIPE_PACKAGE_MANIFEST
        "${FACE_MEDIAPIPE_PACKAGE_DIR}/MANIFEST.txt"
    )

    foreach(required_file IN ITEMS
        "${FACE_MEDIAPIPE_PACKAGE_BRIDGE}"
        "${FACE_MEDIAPIPE_PACKAGE_MANIFEST}"
    )
        if(NOT EXISTS "${required_file}")
            message(FATAL_ERROR
                "Required MediaPipe package file is missing: ${required_file}"
            )
        endif()
    endforeach()

    if(WIN32 AND NOT EXISTS "${FACE_MEDIAPIPE_PACKAGE_OPENCV}")
        message(FATAL_ERROR
            "Required MediaPipe package OpenCV DLL is missing: "
            "${FACE_MEDIAPIPE_PACKAGE_OPENCV}"
        )
    endif()

    file(READ
        "${FACE_MEDIAPIPE_PACKAGE_MANIFEST}"
        FACE_MEDIAPIPE_MANIFEST_CONTENT
    )
    string(FIND
        "${FACE_MEDIAPIPE_MANIFEST_CONTENT}"
        "platform=${FACE_MEDIAPIPE_PACKAGE_PLATFORM}"
        FACE_MEDIAPIPE_MANIFEST_PLATFORM_POSITION
    )
    if(FACE_MEDIAPIPE_MANIFEST_PLATFORM_POSITION EQUAL -1)
        message(FATAL_ERROR
            "MediaPipe package manifest does not declare "
            "platform=${FACE_MEDIAPIPE_PACKAGE_PLATFORM}"
        )
    endif()

    string(FIND
        "${FACE_MEDIAPIPE_MANIFEST_CONTENT}"
        "${FACE_MEDIAPIPE_BRIDGE_NAME}"
        FACE_MEDIAPIPE_MANIFEST_BRIDGE_POSITION
    )
    if(FACE_MEDIAPIPE_MANIFEST_BRIDGE_POSITION EQUAL -1)
        message(FATAL_ERROR
            "MediaPipe package manifest does not list "
            "${FACE_MEDIAPIPE_BRIDGE_NAME}"
        )
    endif()

    message(STATUS
        "MediaPipe runtime package: ${FACE_MEDIAPIPE_PACKAGE_DIR}"
    )
    message(STATUS
        "MediaPipe runtime platform: ${FACE_MEDIAPIPE_PACKAGE_PLATFORM}"
    )

    include(
        "${CMAKE_CURRENT_SOURCE_DIR}/mediapipe/FaceLandmarkerModel.cmake"
    )
    set(FACE_MEDIAPIPE_MODEL_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/models/mediapipe/${FACE_MEDIAPIPE_MODEL_FILENAME}"
        CACHE FILEPATH
        "Path to the pinned MediaPipe Face Landmarker task model"
    )

    if(NOT EXISTS "${FACE_MEDIAPIPE_MODEL_FILE}")
        message(FATAL_ERROR
            "Required MediaPipe model is missing: "
            "${FACE_MEDIAPIPE_MODEL_FILE}"
        )
    endif()

    file(SHA256
        "${FACE_MEDIAPIPE_MODEL_FILE}"
        FACE_MEDIAPIPE_MODEL_ACTUAL_SHA256
    )
    if(NOT
        FACE_MEDIAPIPE_MODEL_ACTUAL_SHA256 STREQUAL
        FACE_MEDIAPIPE_MODEL_SHA256
    )
        message(FATAL_ERROR
            "MediaPipe model SHA-256 mismatch. Expected "
            "${FACE_MEDIAPIPE_MODEL_SHA256}, got "
            "${FACE_MEDIAPIPE_MODEL_ACTUAL_SHA256}: "
            "${FACE_MEDIAPIPE_MODEL_FILE}"
        )
    endif()

    message(STATUS
        "MediaPipe model: ${FACE_MEDIAPIPE_MODEL_FILE}"
    )
    message(STATUS
        "MediaPipe model SHA-256: ${FACE_MEDIAPIPE_MODEL_SHA256}"
    )

endif()


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

add_library(dms_core STATIC
    src/DmsObservation.cpp
    src/DmsScheduler.cpp
    src/DmsEyeMetrics.cpp
    src/DmsTemporalEvents.cpp
    src/DmsPolicy.cpp
    src/RecordedFrameClock.cpp
)

add_library(pfld_landmark_provider STATIC
    src/PfldLandmarkProvider.cpp
    src/PfldEyeLandmarkMapper.cpp
)

add_library(dms_semantic_adapters STATIC
    src/BackendFaceGeometryMapper.cpp
    src/BackendGazeMapper.cpp
    src/DmsEyeQualityAssessor.cpp
    src/DmsHeadPoseEstimator.cpp
)

target_include_directories(dms_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(pfld_landmark_provider
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_include_directories(dms_semantic_adapters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(dms_semantic_adapters PUBLIC ${OpenCV_LIBS})

target_link_libraries(pfld_landmark_provider
    PUBLIC
        ${OpenCV_LIBS}
)

add_executable(yunet_demo

    src/main.cpp
    src/AppPaths.cpp
    src/BackendOptions.cpp
    src/BackendEyeMapper.cpp
    src/FaceDetector.cpp
    src/LbfLandmarkDetector.cpp
    src/LbfEyeLandmarkMapper.cpp
    src/MediaPipeEyeLandmarkMapper.cpp
    src/YuNetLbfBackend.cpp
    src/BlinkTracker.cpp
)

add_executable(face_benchmark
    src/benchmark_main.cpp
    src/AppPaths.cpp
    src/BackendOptions.cpp
    src/BenchmarkOptions.cpp
    src/BackendEyeMapper.cpp
    src/FaceDetector.cpp
    src/LbfLandmarkDetector.cpp
    src/LbfEyeLandmarkMapper.cpp
    src/MediaPipeEyeLandmarkMapper.cpp
    src/YuNetLbfBackend.cpp
    src/YuNetPfldBackend.cpp
    src/BlinkTracker.cpp
)

add_executable(dms_sponsor_selftest
    src/sponsor_selftest.cpp
)
target_link_libraries(dms_sponsor_selftest PRIVATE dms_core)


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

        dms_core
        ${OpenCV_LIBS}
)


# ============================================================
# Model deployment
#
# Legacy models are always copied next to the executable:
#
# Windows:
#   build/Release/models/
#
# Linux / Orin:
#   build/models/
# ============================================================

set(FACE_LEGACY_MODEL_FILES
    face_detection_yunet_2026may.onnx
    lbfmodel.yaml
)

target_include_directories(face_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

foreach(model_file IN LISTS FACE_LEGACY_MODEL_FILES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/models/${model_file}")
        message(FATAL_ERROR
            "Required legacy model is missing: "
            "${CMAKE_CURRENT_SOURCE_DIR}/models/${model_file}"
        )
    endif()
endforeach()

add_custom_command(
    TARGET yunet_demo
    POST_BUILD

    COMMAND
        ${CMAKE_COMMAND}
        -E make_directory
        "$<TARGET_FILE_DIR:yunet_demo>/models"

    COMMENT
        "Creating legacy model deployment directory"
)

foreach(model_file IN LISTS FACE_LEGACY_MODEL_FILES)
    add_custom_command(
        TARGET yunet_demo
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/models/${model_file}"
            "$<TARGET_FILE_DIR:yunet_demo>/models/${model_file}"
        COMMENT
            "Deploying legacy model ${model_file}"
    )
endforeach()

if(FACE_ENABLE_MEDIAPIPE_RUNTIME)
    add_custom_command(
        TARGET yunet_demo
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND}
            -E make_directory
            "$<TARGET_FILE_DIR:yunet_demo>/models/mediapipe"
        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            "${FACE_MEDIAPIPE_MODEL_FILE}"
            "$<TARGET_FILE_DIR:yunet_demo>/models/mediapipe/${FACE_MEDIAPIPE_MODEL_FILENAME}"
        COMMENT
            "Deploying verified MediaPipe model"
    )
endif()


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

    if(FACE_ENABLE_MEDIAPIPE_RUNTIME)
        if(OPENCV_DLL STREQUAL "")
            set(OPENCV_DLL
                "${FACE_MEDIAPIPE_PACKAGE_OPENCV}"
                CACHE FILEPATH
                "Path to OpenCV Windows DLL"
                FORCE
            )
        else()
            get_filename_component(
                FACE_CONFIGURED_OPENCV_DLL
                "${OPENCV_DLL}"
                REALPATH
            )
            get_filename_component(
                FACE_PACKAGED_OPENCV_DLL
                "${FACE_MEDIAPIPE_PACKAGE_OPENCV}"
                REALPATH
            )
            if(NOT FACE_CONFIGURED_OPENCV_DLL STREQUAL FACE_PACKAGED_OPENCV_DLL)
                message(FATAL_ERROR
                    "An enabled MediaPipe package must use its matching "
                    "opencv_world480.dll. Remove OPENCV_DLL or set it to: "
                    "${FACE_MEDIAPIPE_PACKAGE_OPENCV}"
                )
            endif()
        endif()
    endif()


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
# Runtime-loaded MediaPipe C ABI boundary
#
# The application does not link against FaceMediaPipe.dll or
# libFaceMediaPipe.so. Step 13 will wire and stage this loader.
# ============================================================

add_library(face_mediapipe_runtime STATIC
    src/FaceMediaPipeRuntime.cpp
)

target_include_directories(face_mediapipe_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/mediapipe/api
)

target_link_libraries(face_mediapipe_runtime
    PRIVATE
        ${CMAKE_DL_LIBS}
)

target_link_libraries(face_benchmark
    PRIVATE
        dms_core
        dms_semantic_adapters
        pfld_landmark_provider
        ${OpenCV_LIBS}
)

if(WIN32)
    target_link_libraries(face_benchmark PRIVATE Psapi)
endif()

if(FACE_ENABLE_MEDIAPIPE_RUNTIME)

    target_sources(yunet_demo
        PRIVATE
            src/MediaPipeBackend.cpp
    )

    target_sources(face_benchmark
        PRIVATE
            src/MediaPipeBackend.cpp
    )

    target_compile_definitions(yunet_demo
        PRIVATE
            FACE_MEDIAPIPE_RUNTIME_ENABLED=1
    )

    target_compile_definitions(face_benchmark
        PRIVATE
            FACE_MEDIAPIPE_RUNTIME_ENABLED=1
    )

    target_link_libraries(yunet_demo
        PRIVATE
            face_mediapipe_runtime
    )

    target_link_libraries(face_benchmark
        PRIVATE
            face_mediapipe_runtime
    )

    add_custom_command(
        TARGET yunet_demo
        POST_BUILD

        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            "${FACE_MEDIAPIPE_PACKAGE_BRIDGE}"
            "$<TARGET_FILE_DIR:yunet_demo>/${FACE_MEDIAPIPE_BRIDGE_NAME}"

        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            "${FACE_MEDIAPIPE_PACKAGE_MANIFEST}"
            "$<TARGET_FILE_DIR:yunet_demo>/FaceMediaPipe.MANIFEST.txt"

        COMMENT
            "Staging packaged MediaPipe runtime"
    )

endif()


# ============================================================
# Focused blink-processing tests
# ============================================================

if(BUILD_TESTING)

    add_test(
        NAME dms_sponsor_selftest
        COMMAND dms_sponsor_selftest
            "${CMAKE_CURRENT_SOURCE_DIR}/demo/synthetic_eye_sequence.csv"
    )

    add_executable(dms_eye_metrics_test
        tests/dms_eye_metrics_test.cpp
    )
    target_link_libraries(dms_eye_metrics_test PRIVATE dms_core)
    add_test(NAME dms_eye_metrics_test COMMAND dms_eye_metrics_test)

    add_executable(backend_face_geometry_mapper_test
        tests/backend_face_geometry_mapper_test.cpp
    )
    target_link_libraries(backend_face_geometry_mapper_test PRIVATE dms_semantic_adapters)
    add_test(NAME backend_face_geometry_mapper_test COMMAND backend_face_geometry_mapper_test)

    add_executable(dms_head_pose_estimator_test
        tests/dms_head_pose_estimator_test.cpp
    )
    target_link_libraries(dms_head_pose_estimator_test PRIVATE dms_semantic_adapters)
    add_test(NAME dms_head_pose_estimator_test COMMAND dms_head_pose_estimator_test)

    add_executable(backend_gaze_mapper_test
        tests/backend_gaze_mapper_test.cpp
    )
    target_link_libraries(backend_gaze_mapper_test PRIVATE dms_semantic_adapters)
    add_test(NAME backend_gaze_mapper_test COMMAND backend_gaze_mapper_test)

    add_executable(dms_eye_quality_assessor_test
        tests/dms_eye_quality_assessor_test.cpp
    )
    target_link_libraries(dms_eye_quality_assessor_test PRIVATE dms_semantic_adapters)
    add_test(NAME dms_eye_quality_assessor_test COMMAND dms_eye_quality_assessor_test)

    add_executable(dms_temporal_events_test
        tests/dms_temporal_events_test.cpp
    )
    target_link_libraries(dms_temporal_events_test PRIVATE dms_core)
    add_test(NAME dms_temporal_events_test COMMAND dms_temporal_events_test)

    add_executable(dms_policy_test tests/dms_policy_test.cpp)
    target_link_libraries(dms_policy_test PRIVATE dms_core)
    add_test(NAME dms_policy_test COMMAND dms_policy_test)

    add_executable(recorded_frame_clock_test
        tests/recorded_frame_clock_test.cpp
    )
    target_link_libraries(recorded_frame_clock_test PRIVATE dms_core)
    add_test(NAME recorded_frame_clock_test COMMAND recorded_frame_clock_test)

    set(FACE_TEST_IMAGE
        ""
        CACHE FILEPATH
        "Optional face image used by the legacy blink integration test"
    )

    set(FACE_PFLD_MODEL
        ""
        CACHE FILEPATH
        "Optional checksum-pinned PFLD ONNX model for provider integration testing"
    )

    set(FACE_PFLD_MODEL_SHA256
        "7d7bbd5c6a1d9272e58d9773898284a1905d872eba9a662df9b5f20f1ba6f83e"
    )

    add_executable(pfld_eye_landmark_mapper_test
        tests/pfld_eye_landmark_mapper_test.cpp
    )
    target_link_libraries(pfld_eye_landmark_mapper_test
        PRIVATE pfld_landmark_provider
    )
    add_test(
        NAME pfld_eye_landmark_mapper_test
        COMMAND pfld_eye_landmark_mapper_test
    )

    if(NOT FACE_PFLD_MODEL STREQUAL "")
        if(NOT EXISTS "${FACE_PFLD_MODEL}")
            message(FATAL_ERROR "FACE_PFLD_MODEL does not exist: ${FACE_PFLD_MODEL}")
        endif()
        file(SHA256 "${FACE_PFLD_MODEL}" FACE_PFLD_MODEL_ACTUAL_SHA256)
        if(NOT FACE_PFLD_MODEL_ACTUAL_SHA256 STREQUAL FACE_PFLD_MODEL_SHA256)
            message(FATAL_ERROR
                "PFLD model checksum mismatch: expected ${FACE_PFLD_MODEL_SHA256}, "
                "got ${FACE_PFLD_MODEL_ACTUAL_SHA256}"
            )
        endif()
    endif()

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

    add_executable(mediapipe_eye_landmark_mapper_test
        tests/mediapipe_eye_landmark_mapper_test.cpp
        src/MediaPipeEyeLandmarkMapper.cpp
    )

    target_include_directories(mediapipe_eye_landmark_mapper_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(mediapipe_eye_landmark_mapper_test
        PRIVATE
            ${OpenCV_LIBS}
    )

    add_test(
        NAME mediapipe_eye_landmark_mapper_test
        COMMAND mediapipe_eye_landmark_mapper_test
    )

    add_executable(backend_options_test
        tests/backend_options_test.cpp
        src/BackendOptions.cpp
    )
    target_include_directories(backend_options_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(
        NAME backend_options_test
        COMMAND backend_options_test
    )

    add_executable(benchmark_options_test
        tests/benchmark_options_test.cpp
        src/BenchmarkOptions.cpp
    )
    target_include_directories(benchmark_options_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(
        NAME benchmark_options_test
        COMMAND benchmark_options_test
    )

    add_executable(dms_core_test
        tests/dms_core_test.cpp
    )
    target_link_libraries(dms_core_test
        PRIVATE dms_core
    )
    add_test(
        NAME dms_core_test
        COMMAND dms_core_test
    )

    add_executable(monotonic_timestamp_test
        mediapipe/tests/monotonic_timestamp_test.cpp
    )
    target_include_directories(monotonic_timestamp_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/mediapipe/src
    )
    add_test(
        NAME monotonic_timestamp_test
        COMMAND monotonic_timestamp_test
    )

    foreach(mock_target IN ITEMS
        face_mediapipe_mock_valid
        face_mediapipe_mock_missing_symbol
        face_mediapipe_mock_wrong_version
    )
        add_library(${mock_target} SHARED
            tests/face_mediapipe_mock.cpp
        )
        target_include_directories(${mock_target}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/mediapipe/api
        )
        target_compile_definitions(${mock_target}
            PRIVATE
                FACE_MEDIAPIPE_BUILD
        )
    endforeach()

    target_compile_definitions(face_mediapipe_mock_missing_symbol
        PRIVATE FACE_MP_MOCK_MISSING_DESTROY
    )
    target_compile_definitions(face_mediapipe_mock_wrong_version
        PRIVATE FACE_MP_MOCK_WRONG_VERSION
    )

    add_executable(face_mediapipe_runtime_test
        tests/face_mediapipe_runtime_test.cpp
    )
    target_link_libraries(face_mediapipe_runtime_test
        PRIVATE face_mediapipe_runtime
    )

    add_test(
        NAME face_mediapipe_runtime_test
        COMMAND face_mediapipe_runtime_test
            $<TARGET_FILE:face_mediapipe_mock_valid>
            $<TARGET_FILE:face_mediapipe_mock_missing_symbol>
            $<TARGET_FILE:face_mediapipe_mock_wrong_version>
    )

    add_executable(face_mediapipe_runtime_probe
        tests/face_mediapipe_runtime_probe.cpp
    )
    target_link_libraries(face_mediapipe_runtime_probe
        PRIVATE face_mediapipe_runtime
    )

    add_executable(face_backend_integration_test
        tests/face_backend_integration_test.cpp
        src/BackendOptions.cpp
        src/BackendEyeMapper.cpp
        src/FaceDetector.cpp
        src/LbfLandmarkDetector.cpp
        src/LbfEyeLandmarkMapper.cpp
        src/MediaPipeEyeLandmarkMapper.cpp
        src/YuNetLbfBackend.cpp
        src/BlinkTracker.cpp
    )
    target_include_directories(face_backend_integration_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(face_backend_integration_test
        PRIVATE
            ${OpenCV_LIBS}
    )

    if(FACE_ENABLE_MEDIAPIPE_RUNTIME)
        target_sources(face_backend_integration_test
            PRIVATE
                src/MediaPipeBackend.cpp
        )
        target_compile_definitions(face_backend_integration_test
            PRIVATE
                FACE_MEDIAPIPE_RUNTIME_ENABLED=1
        )
        target_link_libraries(face_backend_integration_test
            PRIVATE
                face_mediapipe_runtime
        )
    endif()

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

    add_executable(pfld_landmark_provider_test
        tests/pfld_landmark_provider_test.cpp
        src/FaceDetector.cpp
    )
    target_include_directories(pfld_landmark_provider_test
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(pfld_landmark_provider_test
        PRIVATE
            pfld_landmark_provider
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

        add_test(
            NAME yunet_backend_integration_test
            COMMAND face_backend_integration_test
                yunet
                "${FACE_TEST_IMAGE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/models/face_detection_yunet_2026may.onnx"
                "${CMAKE_CURRENT_SOURCE_DIR}/models/lbfmodel.yaml"
        )

        if(NOT FACE_PFLD_MODEL STREQUAL "")
            add_test(
                NAME pfld_landmark_provider_test
                COMMAND pfld_landmark_provider_test
                    "${CMAKE_CURRENT_SOURCE_DIR}/models/face_detection_yunet_2026may.onnx"
                    "${FACE_PFLD_MODEL}"
                    "${FACE_TEST_IMAGE}"
            )
        endif()

        if(FACE_ENABLE_MEDIAPIPE_RUNTIME)
            add_test(
                NAME mediapipe_backend_integration_test
                COMMAND face_backend_integration_test
                    mediapipe
                    "${FACE_TEST_IMAGE}"
                    "$<TARGET_FILE_DIR:yunet_demo>/${FACE_MEDIAPIPE_BRIDGE_NAME}"
                    "$<TARGET_FILE_DIR:yunet_demo>/models/mediapipe/${FACE_MEDIAPIPE_MODEL_FILENAME}"
            )
        endif()

    endif()

endif()
