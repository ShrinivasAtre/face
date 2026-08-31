#!/usr/bin/env bash
set -euo pipefail

PACKAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND="${1:-mediapipe}"
VIDEO="${2:-}"
if [[ "${BACKEND}" != "yunet" && "${BACKEND}" != "mediapipe" ]]; then
    echo 'Usage: ./run_video_demo.sh [yunet|mediapipe] [video-path]' >&2
    exit 2
fi
if [[ -z "${VIDEO}" ]]; then
    mapfile -t VIDEOS < <(find "${PACKAGE_ROOT}/videos" -type f \( -iname '*.mp4' -o -iname '*.avi' -o -iname '*.mov' -o -iname '*.mkv' \) | LC_ALL=C sort)
    if [[ ${#VIDEOS[@]} -eq 0 ]]; then echo 'No demonstration videos are installed.' >&2; exit 3; fi
    echo 'Available demonstration videos:'
    for index in "${!VIDEOS[@]}"; do printf '  [%d] %s\n' "$((index + 1))" "$(basename "${VIDEOS[index]}")"; done
    read -r -p 'Select video number: ' SELECTION
    if ! [[ "${SELECTION}" =~ ^[0-9]+$ ]] || (( SELECTION < 1 || SELECTION > ${#VIDEOS[@]} )); then
        echo 'Invalid video selection.' >&2; exit 4
    fi
    VIDEO="${VIDEOS[SELECTION - 1]}"
elif [[ "${VIDEO}" != /* ]]; then
    VIDEO="${PACKAGE_ROOT}/${VIDEO}"
fi
[[ -f "${VIDEO}" ]] || { echo "Video is missing: ${VIDEO}" >&2; exit 5; }
mkdir -p "${PACKAGE_ROOT}/results"
RESULT="${PACKAGE_ROOT}/results/demo-${BACKEND}-$(date +%Y%m%d-%H%M%S).json"
echo "Starting DMS engineering demo: ${BACKEND} / ${VIDEO}"
echo 'Press Q or Esc to stop playback.'
"${PACKAGE_ROOT}/face_benchmark" --backend="${BACKEND}" --input="${VIDEO}" \
    --warmup=10 --output="${RESULT}" --sponsor-demo
echo "Result: ${RESULT}"
