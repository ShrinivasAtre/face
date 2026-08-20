#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "Usage: bash scripts/run_mediapipe_smoke.sh <face_landmarker.task> <image>" >&2
    exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
MODEL_PATH="$(realpath "$1")"
IMAGE_PATH="$(realpath "$2")"
SO_DIR="${REPO_ROOT}/third_party/mediapipe/bazel-bin/face_bridge"
SO_PATH="${SO_DIR}/libFaceMediaPipe.so"
TEST_BIN="${REPO_ROOT}/build/mediapipe_smoke"

if [[ ! -f "${SO_PATH}" ]]; then
    echo "ERROR: ${SO_PATH} not found." >&2
    echo "Run: bash scripts/build_mediapipe.sh" >&2
    exit 3
fi

if [[ ! -f "${MODEL_PATH}" ]]; then
    echo "ERROR: model not found: ${MODEL_PATH}" >&2
    exit 4
fi

if [[ ! -f "${IMAGE_PATH}" ]]; then
    echo "ERROR: image not found: ${IMAGE_PATH}" >&2
    exit 5
fi

if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists opencv4; then
    echo "ERROR: pkg-config could not find opencv4." >&2
    exit 6
fi

mkdir -p "${REPO_ROOT}/build"

CXX="${CXX:-g++}"

"${CXX}" \
    -std=c++17 \
    -O2 \
    -I"${REPO_ROOT}/mediapipe/api" \
    "${REPO_ROOT}/tests/mediapipe_smoke.cpp" \
    $(pkg-config --cflags opencv4) \
    -L"${SO_DIR}" \
    -Wl,-rpath,"${SO_DIR}" \
    -lFaceMediaPipe \
    $(pkg-config --libs opencv4) \
    -o "${TEST_BIN}"

echo "Built ${TEST_BIN}"
echo "Running MediaPipe smoke test..."
"${TEST_BIN}" "${MODEL_PATH}" "${IMAGE_PATH}"
