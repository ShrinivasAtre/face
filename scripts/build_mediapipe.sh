#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MEDIAPIPE_ROOT="${REPO_ROOT}/third_party/mediapipe"
BRIDGE_ROOT="${MEDIAPIPE_ROOT}/face_bridge"
TASKS_CORE_BUILD="${MEDIAPIPE_ROOT}/mediapipe/tasks/cc/core/BUILD"
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
#   1. Remove task_runner's direct analytics enum dependency.
#   2. Remove the dummy logger's unnecessary dependency/include on
#      logging_client (which is the only route to the missing log proto).
# The real logging_client target remains untouched and simply becomes unused
# by the Face Landmarker build path.
if grep -Fq '//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto' "${TASKS_CORE_BUILD}"; then
    echo "Applying MediaPipe v0.10.33 task_runner analytics compatibility patch..."
    sed -i '\|//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto|d' "${TASKS_CORE_BUILD}"
fi

if grep -Fq '":logging_client"' "${TASKS_LOGGING_BUILD}"; then
    echo "Removing dummy logger dependency on unavailable analytics logging client..."
    sed -i '/":logging_client"/d' "${TASKS_LOGGING_BUILD}"
fi

if grep -Fq 'mediapipe/tasks/cc/core/logging/logging_client.h' "${TASKS_DUMMY_LOGGER}"; then
    echo "Removing unused analytics logging_client include from dummy logger..."
    sed -i '\|mediapipe/tasks/cc/core/logging/logging_client.h|d' "${TASKS_DUMMY_LOGGER}"
fi

if grep -Fq '//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto' "${TASKS_CORE_BUILD}"; then
    echo "ERROR: task_runner analytics dependency still present." >&2
    exit 1
fi

rm -rf "${BRIDGE_ROOT}"
mkdir -p "${BRIDGE_ROOT}/api" "${BRIDGE_ROOT}/src"

cp "${REPO_ROOT}/mediapipe/api/FaceMediaPipe.h" "${BRIDGE_ROOT}/api/FaceMediaPipe.h"
cp "${REPO_ROOT}/mediapipe/src/FaceMediaPipe.cpp" "${BRIDGE_ROOT}/src/FaceMediaPipe.cpp"

cat > "${BRIDGE_ROOT}/BUILD.bazel" <<'EOF'
cc_library(
    name = "FaceMediaPipe",
    srcs = ["src/FaceMediaPipe.cpp"],
    hdrs = ["api/FaceMediaPipe.h"],
    deps = [
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
    ],
    copts = ["-DFACE_MEDIAPIPE_BUILD"],
    linkstatic = False,
    visibility = ["//visibility:public"],
)
EOF

cd "${MEDIAPIPE_ROOT}"
bazelisk build //face_bridge:FaceMediaPipe

echo
echo "MediaPipe bridge build completed."
echo "Bazel output: ${MEDIAPIPE_ROOT}/bazel-bin/face_bridge/"
