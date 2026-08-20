#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MEDIAPIPE_ROOT="${REPO_ROOT}/third_party/mediapipe"
BRIDGE_ROOT="${MEDIAPIPE_ROOT}/face_bridge"

if [[ ! -f "${MEDIAPIPE_ROOT}/WORKSPACE" ]]; then
    echo "ERROR: MediaPipe workspace not found at ${MEDIAPIPE_ROOT}" >&2
    echo "Run ./scripts/fetch_mediapipe.sh first." >&2
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
