#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BRIDGE_SO="${REPO_ROOT}/third_party/mediapipe/bazel-bin/face_bridge/libFaceMediaPipe.so"
OUTPUT_DIR="${1:-${REPO_ROOT}/dist/mediapipe/linux-aarch64}"

if [[ ! -f "${BRIDGE_SO}" ]]; then
    echo "ERROR: Required package input not found: ${BRIDGE_SO}" >&2
    exit 1
fi

ARCHITECTURE="$(file -b "${BRIDGE_SO}")"
if [[ "${ARCHITECTURE}" != *"ARM aarch64"* ]]; then
    echo "ERROR: libFaceMediaPipe.so is not ARM aarch64: ${ARCHITECTURE}" >&2
    exit 2
fi

mapfile -t DEPENDENCIES < <(
    readelf -d "${BRIDGE_SO}" |
        sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' |
        sort -u
)

if printf '%s\n' "${DEPENDENCIES[@]}" |
    grep -Eiq 'mediapipe|tensorflow|tflite|protobuf|absl'; then
    echo "ERROR: Bridge has a forbidden Bazel implementation dependency." >&2
    exit 3
fi

mapfile -t EXPORTS < <(
    nm -D --defined-only "${BRIDGE_SO}" |
        awk '{print $3}' |
        grep '^face_mp_' |
        sort -u
)
EXPECTED_EXPORTS=(
    face_mp_api_version
    face_mp_create
    face_mp_destroy
    face_mp_last_error
    face_mp_process_bgr
)
if [[ "$(printf '%s\n' "${EXPORTS[@]}")" != "$(printf '%s\n' "${EXPECTED_EXPORTS[@]}")" ]]; then
    echo "ERROR: Unexpected face_mp exports: ${EXPORTS[*]}" >&2
    exit 4
fi

if ldd "${BRIDGE_SO}" | grep -q 'not found'; then
    echo "ERROR: Unresolved bridge dependencies:" >&2
    ldd "${BRIDGE_SO}" | grep 'not found' >&2
    exit 5
fi

mkdir -p "${OUTPUT_DIR}"
for entry in "${OUTPUT_DIR}"/* "${OUTPUT_DIR}"/.[!.]* "${OUTPUT_DIR}"/..?*; do
    [[ -e "${entry}" ]] || continue
    case "$(basename "${entry}")" in
        libFaceMediaPipe.so|MANIFEST.txt) ;;
        *)
            echo "ERROR: Refusing to clean output containing unexpected entry: ${entry}" >&2
            exit 6
            ;;
    esac
done
rm -f "${OUTPUT_DIR}/libFaceMediaPipe.so" "${OUTPUT_DIR}/MANIFEST.txt"
cp "${BRIDGE_SO}" "${OUTPUT_DIR}/libFaceMediaPipe.so"

PAYLOAD_HASH="$(sha256sum "${OUTPUT_DIR}/libFaceMediaPipe.so" | awk '{print $1}')"
{
    echo 'face-mediapipe-package-v1'
    echo 'platform=linux-aarch64'
    echo 'model_bundled=false'
    echo 'payload_sha256:'
    echo "${PAYLOAD_HASH}  libFaceMediaPipe.so"
    echo 'architecture:'
    echo "${ARCHITECTURE}"
    echo 'direct_dependencies:'
    printf '%s\n' "${DEPENDENCIES[@]}"
    echo 'exports:'
    printf '%s\n' "${EXPORTS[@]}"
    echo 'platform_prerequisites:'
    echo 'NVIDIA JetPack Linux aarch64 target runtime'
    echo 'OpenCV 4.8 shared-library ABI'
    echo 'EGL/GLES target graphics libraries'
    echo 'GNU C/C++ runtime compatible with the build toolchain'
} > "${OUTPUT_DIR}/MANIFEST.txt.tmp"
mv "${OUTPUT_DIR}/MANIFEST.txt.tmp" "${OUTPUT_DIR}/MANIFEST.txt"

echo "MediaPipe Orin package created: ${OUTPUT_DIR}"
cat "${OUTPUT_DIR}/MANIFEST.txt"
