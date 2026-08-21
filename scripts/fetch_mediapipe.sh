#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
VERSION_FILE="${ROOT_DIR}/mediapipe/MEDIAPIPE_VERSION"
MP_DIR="${ROOT_DIR}/third_party/mediapipe"

if [[ ! -f "${VERSION_FILE}" ]]; then
    echo "ERROR: MediaPipe version file not found: ${VERSION_FILE}" >&2
    exit 1
fi
if [[ "$(grep -Ec '^(MEDIAPIPE_TAG|MEDIAPIPE_COMMIT)=[^[:space:]]+$' "${VERSION_FILE}")" -ne 2 ]] ||
   [[ "$(wc -l < "${VERSION_FILE}")" -ne 2 ]]; then
    echo 'ERROR: Invalid mediapipe/MEDIAPIPE_VERSION contract.' >&2
    exit 1
fi
source "${VERSION_FILE}"
EXPECTED_COMMIT="${MEDIAPIPE_COMMIT}"
EXPECTED_TAG="${MEDIAPIPE_TAG}"

if [[ -d "${MP_DIR}/.git" ]]; then
    echo "MediaPipe checkout already exists: ${MP_DIR}"
else
    mkdir -p "${ROOT_DIR}/third_party"
    git clone --filter=blob:none --no-checkout https://github.com/google-ai-edge/mediapipe.git "${MP_DIR}"
fi

git -C "${MP_DIR}" fetch --tags --depth=1 origin "${EXPECTED_TAG}"
git -C "${MP_DIR}" checkout --detach "${EXPECTED_COMMIT}"

ACTUAL_COMMIT="$(git -C "${MP_DIR}" rev-parse HEAD)"
if [[ "${ACTUAL_COMMIT}" != "${EXPECTED_COMMIT}" ]]; then
    echo "ERROR: MediaPipe commit mismatch."
    echo "Expected: ${EXPECTED_COMMIT}"
    echo "Actual:   ${ACTUAL_COMMIT}"
    exit 1
fi

"${SCRIPT_DIR}/verify_mediapipe_dependency.sh"
echo "MediaPipe ${EXPECTED_TAG} (${ACTUAL_COMMIT}) is ready in ${MP_DIR}"
