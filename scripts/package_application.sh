#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: ./scripts/package_application.sh <cmake-build-dir> [output-dir]" >&2
    exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SOURCE_REVISION="$(git -c safe.directory="${REPO_ROOT}" -C "${REPO_ROOT}" rev-parse HEAD)"
BUILD_DIR="$(realpath "$1")"
OUTPUT_DIR="${2:-${REPO_ROOT}/dist/application/linux-aarch64}"
OUTPUT_DIR="$(realpath -m "${OUTPUT_DIR}")"
if [[ "${OUTPUT_DIR}" == "/" || "${OUTPUT_DIR}" == "${REPO_ROOT}" ]]; then
    echo "ERROR: Unsafe application package output directory: ${OUTPUT_DIR}" >&2
    exit 3
fi

declare -A INPUTS=(
    [yunet_demo]="${BUILD_DIR}/yunet_demo"
    [dms_sponsor_selftest]="${BUILD_DIR}/dms_sponsor_selftest"
    [libFaceMediaPipe.so]="${BUILD_DIR}/libFaceMediaPipe.so"
    [FaceMediaPipe.MANIFEST.txt]="${BUILD_DIR}/FaceMediaPipe.MANIFEST.txt"
    [models/face_detection_yunet_2026may.onnx]="${BUILD_DIR}/models/face_detection_yunet_2026may.onnx"
    [models/lbfmodel.yaml]="${BUILD_DIR}/models/lbfmodel.yaml"
    [models/mediapipe/face_landmarker.task]="${BUILD_DIR}/models/mediapipe/face_landmarker.task"
    [run_face.sh]="${SCRIPT_DIR}/run_deployed_face.sh"
    [run_self_test.sh]="${SCRIPT_DIR}/run_sponsor_selftest.sh"
    [test-data/synthetic_eye_sequence.csv]="${REPO_ROOT}/demo/synthetic_eye_sequence.csv"
    [README_SPONSOR_DEMO.md]="${REPO_ROOT}/docs/SPONSOR_DEMO.md"
)
for relative in "${!INPUTS[@]}"; do
    if [[ ! -f "${INPUTS[${relative}]}" ]]; then
        echo "ERROR: Required application package input is missing: ${INPUTS[${relative}]}" >&2
        exit 4
    fi
done

for binary in "${INPUTS[yunet_demo]}" "${INPUTS[libFaceMediaPipe.so]}"; do
    if ! file "${binary}" | grep -q 'ARM aarch64'; then
        echo "ERROR: Expected an ARM aarch64 binary: ${binary}" >&2
        exit 5
    fi
done
if readelf -d "${INPUTS[yunet_demo]}" | grep -q 'Shared library: \[libFaceMediaPipe.so\]'; then
    echo 'ERROR: The application links directly to libFaceMediaPipe.so.' >&2
    exit 6
fi
if ! grep -qx 'platform=linux-aarch64' "${INPUTS[FaceMediaPipe.MANIFEST.txt]}" ||
   ! grep -qx 'model_bundled=false' "${INPUTS[FaceMediaPipe.MANIFEST.txt]}"; then
    echo 'ERROR: Bridge manifest does not match the Orin package boundary.' >&2
    exit 7
fi
EXPECTED_MODEL_HASH='64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff'
ACTUAL_MODEL_HASH="$(sha256sum "${INPUTS[models/mediapipe/face_landmarker.task]}" | awk '{print $1}')"
if [[ "${ACTUAL_MODEL_HASH}" != "${EXPECTED_MODEL_HASH}" ]]; then
    echo "ERROR: Face Landmarker model hash mismatch: ${ACTUAL_MODEL_HASH}" >&2
    exit 8
fi

rm -rf -- "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"
for relative in "${!INPUTS[@]}"; do
    mkdir -p "$(dirname "${OUTPUT_DIR}/${relative}")"
    cp "${INPUTS[${relative}]}" "${OUTPUT_DIR}/${relative}"
done
chmod 755 "${OUTPUT_DIR}/yunet_demo" "${OUTPUT_DIR}/dms_sponsor_selftest" \
    "${OUTPUT_DIR}/run_face.sh" "${OUTPUT_DIR}/run_self_test.sh"

mapfile -t HASH_LINES < <(
    cd "${OUTPUT_DIR}"
    find . -type f ! -name APPLICATION_MANIFEST.txt -printf '%P\n' |
        LC_ALL=C sort |
        while IFS= read -r relative; do
            hash="$(sha256sum "${relative}" | awk '{print $1}')"
            printf '%s  %s\n' "${hash}" "${relative}"
        done
)
mapfile -t DEPENDENCIES < <(
    readelf -d "${INPUTS[yunet_demo]}" |
        sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' |
        LC_ALL=C sort -u
)
{
    echo 'face-application-package-v2-sponsor-demo'
    echo 'platform=linux-aarch64'
    echo 'architecture=ARM aarch64'
    echo "source_revision=${SOURCE_REVISION}"
    echo 'backends=yunet,mediapipe'
    echo 'mediapipe_model=models/mediapipe/face_landmarker.task'
    echo 'payload_sha256:'
    printf '%s\n' "${HASH_LINES[@]}"
    echo 'application_dependencies:'
    printf '%s\n' "${DEPENDENCIES[@]}"
    echo 'launch:'
    echo './run_face.sh yunet'
    echo './run_face.sh mediapipe'
    echo './run_self_test.sh'
    echo 'platform_prerequisites:'
    echo 'JetPack-compatible Linux aarch64 runtime'
    echo 'OpenCV 4.8 shared libraries'
    echo 'EGL/GLES and JetPack multimedia libraries required by the bridge'
} > "${OUTPUT_DIR}/APPLICATION_MANIFEST.txt"

echo 'Final Orin application package completed.'
echo "Output: ${OUTPUT_DIR}"
