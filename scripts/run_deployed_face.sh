#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ( "$1" != "yunet" && "$1" != "mediapipe" ) ]]; then
    echo "Usage: ./run_face.sh <yunet|mediapipe>" >&2
    exit 2
fi

DEPLOY_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ ! -x "${DEPLOY_DIR}/yunet_demo" ]]; then
    echo "ERROR: Deployed application not found: ${DEPLOY_DIR}/yunet_demo" >&2
    exit 3
fi

cd "${DEPLOY_DIR}"
exec ./yunet_demo "--backend=$1"
