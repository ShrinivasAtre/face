#!/usr/bin/env bash
set -euo pipefail
PACKAGE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST="${PACKAGE_ROOT}/APPLICATION_MANIFEST.txt"
[[ -f "${MANIFEST}" ]] || { echo "Manifest is missing: ${MANIFEST}" >&2; exit 2; }
checked=0
in_payload=0
while IFS= read -r line; do
    if [[ "${line}" == 'payload_sha256:' ]]; then in_payload=1; continue; fi
    if [[ "${line}" == 'application_dependencies:' ]]; then break; fi
    if (( in_payload )); then
        hash="${line%%  *}"
        relative="${line#*  }"
        [[ "${hash}" =~ ^[0-9a-f]{64}$ && "${relative}" != "${line}" ]] || {
            echo "Invalid payload record: ${line}" >&2; exit 3; }
        [[ -f "${PACKAGE_ROOT}/${relative}" ]] || { echo "Payload is missing: ${relative}" >&2; exit 4; }
        actual="$(sha256sum "${PACKAGE_ROOT}/${relative}" | awk '{print $1}')"
        [[ "${actual}" == "${hash}" ]] || { echo "Payload checksum mismatch: ${relative}" >&2; exit 5; }
        ((checked += 1))
    fi
done < "${MANIFEST}"
(( checked > 0 )) || { echo 'Manifest contains no payload records.' >&2; exit 6; }
echo "PACKAGE VERIFY PASSED (${checked} files)"
