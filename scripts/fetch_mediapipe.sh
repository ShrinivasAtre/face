#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION_FILE="${ROOT_DIR}/mediapipe/MEDIAPIPE_VERSION"
MP_DIR="${ROOT_DIR}/third_party/mediapipe"

EXPECTED_COMMIT="3987048"
EXPECTED_TAG="v0.10.33"

if [[ -d "${MP_DIR}/.git" ]]; then
    echo "MediaPipe checkout already exists: ${MP_DIR}"
else
    mkdir -p "${ROOT_DIR}/third_party"
    git clone --filter=blob:none --no-checkout https://github.com/google-ai-edge/mediapipe.git "${MP_DIR}"
fi

git -C "${MP_DIR}" fetch --tags --depth=1 origin "${EXPECTED_TAG}"
git -C "${MP_DIR}" checkout --detach "${EXPECTED_TAG}"

ACTUAL_COMMIT="$(git -C "${MP_DIR}" rev-parse --short=7 HEAD)"
if [[ "${ACTUAL_COMMIT}" != "${EXPECTED_COMMIT}" ]]; then
    echo "ERROR: MediaPipe commit mismatch."
    echo "Expected: ${EXPECTED_COMMIT}"
    echo "Actual:   ${ACTUAL_COMMIT}"
    exit 1
fi

echo "MediaPipe ${EXPECTED_TAG} (${ACTUAL_COMMIT}) is ready in ${MP_DIR}"
