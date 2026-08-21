#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
VERSION_FILE="${REPO_ROOT}/mediapipe/MEDIAPIPE_VERSION"
BAZEL_VERSION_FILE="${REPO_ROOT}/.bazelversion"
MEDIAPIPE_ROOT="${REPO_ROOT}/third_party/mediapipe"
EXPECTED_ORIGIN="https://github.com/google-ai-edge/mediapipe.git"

if [[ ! -f "${VERSION_FILE}" || ! -f "${BAZEL_VERSION_FILE}" ]]; then
    echo 'ERROR: Required dependency contract file is missing.' >&2
    exit 1
fi

# The committed file contains only these two shell-compatible assignments.
# Reject extra or malformed content before sourcing it.
if [[ "$(grep -Ec '^(MEDIAPIPE_TAG|MEDIAPIPE_COMMIT)=[^[:space:]]+$' "${VERSION_FILE}")" -ne 2 ]] ||
   [[ "$(wc -l < "${VERSION_FILE}")" -ne 2 ]]; then
    echo 'ERROR: MEDIAPIPE_VERSION must contain exactly MEDIAPIPE_TAG and MEDIAPIPE_COMMIT.' >&2
    exit 2
fi
source "${VERSION_FILE}"

if [[ ! "${MEDIAPIPE_TAG}" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]] ||
   [[ ! "${MEDIAPIPE_COMMIT}" =~ ^[0-9a-f]{40}$ ]]; then
    echo 'ERROR: Invalid MediaPipe tag or full commit hash.' >&2
    exit 3
fi

BAZEL_VERSION="$(tr -d '[:space:]' < "${BAZEL_VERSION_FILE}")"
if [[ ! "${BAZEL_VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "ERROR: Invalid .bazelversion: ${BAZEL_VERSION}" >&2
    exit 4
fi
if ! command -v bazelisk >/dev/null 2>&1; then
    echo 'ERROR: bazelisk was not found on PATH.' >&2
    exit 5
fi
BAZEL_OUTPUT="$(bazelisk version 2>&1)"
if ! grep -Fxq "Build label: ${BAZEL_VERSION}" <<< "${BAZEL_OUTPUT}"; then
    echo "ERROR: Effective Bazel version does not match ${BAZEL_VERSION}." >&2
    echo "${BAZEL_OUTPUT}" >&2
    exit 6
fi

if [[ ! -d "${MEDIAPIPE_ROOT}/.git" || ! -f "${MEDIAPIPE_ROOT}/WORKSPACE" ]]; then
    echo "ERROR: Verified MediaPipe workspace not found at ${MEDIAPIPE_ROOT}." >&2
    echo 'Run ./scripts/fetch_mediapipe.sh.' >&2
    exit 7
fi

ORIGIN="$(git -C "${MEDIAPIPE_ROOT}" remote get-url origin)"
HEAD_COMMIT="$(git -C "${MEDIAPIPE_ROOT}" rev-parse HEAD)"
TAG_COMMIT="$(git -C "${MEDIAPIPE_ROOT}" rev-parse "${MEDIAPIPE_TAG}^{commit}")"

if [[ "${ORIGIN}" != "${EXPECTED_ORIGIN}" ]]; then
    echo "ERROR: MediaPipe origin mismatch. Expected ${EXPECTED_ORIGIN}, got ${ORIGIN}." >&2
    exit 8
fi
if [[ "${HEAD_COMMIT}" != "${MEDIAPIPE_COMMIT}" ]]; then
    echo "ERROR: MediaPipe commit mismatch. Expected ${MEDIAPIPE_COMMIT}, got ${HEAD_COMMIT}." >&2
    exit 9
fi
if [[ "${TAG_COMMIT}" != "${MEDIAPIPE_COMMIT}" ]]; then
    echo "ERROR: MediaPipe tag ${MEDIAPIPE_TAG} does not resolve to ${MEDIAPIPE_COMMIT}." >&2
    exit 10
fi

echo 'MediaPipe dependency verification PASSED.'
echo "Bazel: ${BAZEL_VERSION}"
echo "MediaPipe: ${MEDIAPIPE_TAG} (${HEAD_COMMIT})"
echo "Origin: ${ORIGIN}"
