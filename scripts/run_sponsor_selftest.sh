#!/usr/bin/env bash
set -euo pipefail
DEPLOY_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "${DEPLOY_DIR}/dms_sponsor_selftest" "${DEPLOY_DIR}/test-data/synthetic_eye_sequence.csv"
