#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MEDIAPIPE_ROOT="${REPO_ROOT}/third_party/mediapipe"
BRIDGE_ROOT="${MEDIAPIPE_ROOT}/face_bridge"
TASKS_CORE_BUILD="${MEDIAPIPE_ROOT}/mediapipe/tasks/cc/core/BUILD"
TASKS_CORE_SRC="${MEDIAPIPE_ROOT}/mediapipe/tasks/cc/core/task_runner.cc"
TASKS_LOGGING_BUILD="${MEDIAPIPE_ROOT}/mediapipe/tasks/cc/core/logging/BUILD"
TASKS_DUMMY_LOGGER="${MEDIAPIPE_ROOT}/mediapipe/tasks/cc/core/logging/tasks_dummy_logger.h"

if [[ ! -f "${MEDIAPIPE_ROOT}/WORKSPACE" ]]; then
    echo "ERROR: MediaPipe workspace not found at ${MEDIAPIPE_ROOT}" >&2
    echo "Run ./scripts/fetch_mediapipe.sh first." >&2
    exit 1
fi

# MediaPipe v0.10.33 references internal analytics protos that are not shipped
# in the OSS checkout. The Face Landmarker path uses the dummy logger, which
# does not need those analytics types. Apply the smallest compatibility patch:
#   1. Remove task_runner's direct analytics enum Bazel dependency.
#   2. Remove task_runner's now-unused analytics enum include.
#   3. Remove the dummy logger's unnecessary dependency/include on
#      logging_client (which is the only route to the missing log proto).
# The real logging_client target remains untouched and simply becomes unused
# by the Face Landmarker build path.
if grep -Fq '//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto' "${TASKS_CORE_BUILD}"; then
    echo "Applying MediaPipe v0.10.33 task_runner analytics compatibility patch..."
    sed -i '\|//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto|d' "${TASKS_CORE_BUILD}"
fi

if grep -Fq 'mediapipe/util/analytics/mediapipe_logging_enums.pb.h' "${TASKS_CORE_SRC}"; then
    echo "Removing unused task_runner analytics enum include..."
    sed -i '\|mediapipe/util/analytics/mediapipe_logging_enums.pb.h|d' "${TASKS_CORE_SRC}"
fi

if grep -Fq '":logging_client"' "${TASKS_LOGGING_BUILD}"; then
    echo "Removing dummy logger dependency on unavailable analytics logging client..."
    sed -i '/":logging_client"/d' "${TASKS_LOGGING_BUILD}"
fi

if grep -Fq 'mediapipe/tasks/cc/core/logging/logging_client.h' "${TASKS_DUMMY_LOGGER}"; then
    echo "Removing unused analytics logging_client include from dummy logger..."
    sed -i '\|mediapipe/tasks/cc/core/logging/logging_client.h|d' "${TASKS_DUMMY_LOGGER}"
fi

if grep -Fq '//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto' "${TASKS_CORE_BUILD}" || \
   grep -Fq 'mediapipe/util/analytics/mediapipe_logging_enums.pb.h' "${TASKS_CORE_SRC}"; then
    echo "ERROR: task_runner analytics dependency/include still present." >&2
    exit 1
fi

rm -rf "${BRIDGE_ROOT}"
mkdir -p "${BRIDGE_ROOT}/api" "${BRIDGE_ROOT}/src" "${BRIDGE_ROOT}/tests"

cp "${REPO_ROOT}/mediapipe/api/FaceMediaPipe.h" "${BRIDGE_ROOT}/api/FaceMediaPipe.h"
cp "${REPO_ROOT}/mediapipe/src/FaceMediaPipe.cpp" "${BRIDGE_ROOT}/src/FaceMediaPipe.cpp"
cp "${REPO_ROOT}/mediapipe/src/BgrToRgb.cpp" "${BRIDGE_ROOT}/src/BgrToRgb.cpp"
cp "${REPO_ROOT}/mediapipe/src/BgrToRgb.h" "${BRIDGE_ROOT}/src/BgrToRgb.h"
cp "${REPO_ROOT}/mediapipe/src/LandmarkConversion.cpp" "${BRIDGE_ROOT}/src/LandmarkConversion.cpp"
cp "${REPO_ROOT}/mediapipe/src/LandmarkConversion.h" "${BRIDGE_ROOT}/src/LandmarkConversion.h"
cp "${REPO_ROOT}/tests/bgr_to_rgb_test.cpp" "${BRIDGE_ROOT}/tests/bgr_to_rgb_test.cpp"
cp "${REPO_ROOT}/tests/landmark_conversion_test.cpp" "${BRIDGE_ROOT}/tests/landmark_conversion_test.cpp"

# A cc_library with linkstatic=False can produce a .so that still contains
# unresolved references to its transitive MediaPipe dependencies. Build the
# wrapper as an always-linked implementation library and then link a true
# shared binary with linkstatic=True so MediaPipe/Abseil/TFLite objects are
# pulled into libFaceMediaPipe.so. --no-undefined makes this property explicit
# and causes the Bazel build itself to fail if anything is still unresolved.
cat > "${BRIDGE_ROOT}/BUILD.bazel" <<'EOF'
cc_library(
    name = "FaceMediaPipe_impl",
    srcs = [
        "src/BgrToRgb.cpp",
        "src/LandmarkConversion.cpp",
        "src/FaceMediaPipe.cpp",
    ],
    hdrs = [
        "api/FaceMediaPipe.h",
        "src/BgrToRgb.h",
        "src/LandmarkConversion.h",
    ],
    deps = [
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
    ],
    copts = ["-DFACE_MEDIAPIPE_BUILD"],
    alwayslink = True,
)

cc_binary(
    name = "libFaceMediaPipe.so",
    deps = [":FaceMediaPipe_impl"],
    linkshared = True,
    linkstatic = True,
    linkopts = ["-Wl,--no-undefined"],
    visibility = ["//visibility:public"],
)
cc_test(
    name = "bgr_to_rgb_test",
    srcs = [
        "src/BgrToRgb.cpp",
        "src/BgrToRgb.h",
        "tests/bgr_to_rgb_test.cpp",
    ],
    includes = ["src"],
)
cc_test(
    name = "landmark_conversion_test",
    srcs = [
        "src/LandmarkConversion.cpp",
        "src/LandmarkConversion.h",
        "tests/landmark_conversion_test.cpp",
    ],
    includes = [
        "api",
        "src",
    ],
)
EOF

cd "${MEDIAPIPE_ROOT}"

BAZEL_ARGS=("//face_bridge:libFaceMediaPipe.so")

# Jetson Orin systems commonly have 8 GiB RAM and no swap. The first
# MediaPipe/TensorFlow build can otherwise start too many large C++ compiler
# processes at once. Keep the Linux/aarch64 build conservative and
# reproducible. Bazel's cache still makes subsequent builds much faster.
if [[ "$(uname -s)" == "Linux" && "$(uname -m)" == "aarch64" ]]; then
    echo "Using conservative Orin Bazel limits: jobs=2, local RAM=4096 MB"
    BAZEL_ARGS+=("--jobs=2" "--local_ram_resources=4096")
fi

# MediaPipe v0.10.33's Linux OpenCV BUILD rule assumes an apt-style OpenCV 4
# installation rooted under /usr, but its OpenCV 4 include entries are
# commented out. Detect the actual header location instead of relying on a
# potentially stale pkg-config cflags entry, then pass the include path to all
# C++ compilations. Also preserve pkg-config's library search path when one is
# supplied (for example /usr/local/lib on the Orin image).
if [[ "$(uname -s)" == "Linux" ]]; then
    OPENCV_INCLUDE=""
    for candidate in \
        /usr/include/opencv4 \
        /usr/local/include/opencv4 \
        /usr/local/opencv-4.8.0-contrib/include/opencv4; do
        if [[ -f "${candidate}/opencv2/core/version.hpp" ]]; then
            OPENCV_INCLUDE="${candidate}"
            break
        fi
    done

    if [[ -z "${OPENCV_INCLUDE}" ]]; then
        echo "ERROR: Could not locate OpenCV header opencv2/core/version.hpp" >&2
        exit 1
    fi

    echo "Using OpenCV headers from ${OPENCV_INCLUDE}"
    BAZEL_ARGS+=("--copt=-I${OPENCV_INCLUDE}")

    if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists opencv4; then
        while IFS= read -r token; do
            if [[ "${token}" == -L* ]]; then
                echo "Using OpenCV library search path ${token#-L}"
                BAZEL_ARGS+=("--linkopt=${token}")
            fi
        done < <(pkg-config --libs-only-L opencv4 | tr ' ' '\n' | sed '/^$/d')
    fi
fi

bazelisk build "${BAZEL_ARGS[@]}"

echo
echo "MediaPipe bridge build completed."
echo "Bazel output: ${MEDIAPIPE_ROOT}/bazel-bin/face_bridge/libFaceMediaPipe.so"
