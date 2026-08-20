#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MEDIAPIPE_ROOT="${REPO_ROOT}/third_party/mediapipe"
BRIDGE_ROOT="${MEDIAPIPE_ROOT}/face_bridge"
TASKS_CORE_BUILD="${MEDIAPIPE_ROOT}/mediapipe/tasks/cc/core/BUILD"
ANALYTICS_DEP='        "//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto",'

if [[ ! -f "${MEDIAPIPE_ROOT}/WORKSPACE" ]]; then
    echo "ERROR: MediaPipe workspace not found at ${MEDIAPIPE_ROOT}" >&2
    echo "Run ./scripts/fetch_mediapipe.sh first." >&2
    exit 1
fi

# MediaPipe v0.10.33 references an internal analytics proto from task_runner,
# but mediapipe/util/analytics is not included in the OSS checkout. Apply the
# smallest possible compatibility patch and only when the exact dependency is
# present. The fetched checkout remains pinned; fetch_mediapipe.sh can restore
# it from Git at any time.
if grep -Fqx "${ANALYTICS_DEP}" "${TASKS_CORE_BUILD}"; then
    echo "Applying MediaPipe v0.10.33 OSS analytics compatibility patch..."
    sed -i '\|//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto|d' "${TASKS_CORE_BUILD}"
fi

if grep -Fq '//mediapipe/util/analytics:' "${TASKS_CORE_BUILD}"; then
    echo "ERROR: Unexpected MediaPipe analytics dependency remains in ${TASKS_CORE_BUILD}" >&2
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
