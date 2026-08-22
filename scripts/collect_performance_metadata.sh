#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 <output-file>" >&2
  exit 2
fi

output_file=$1
mkdir -p "$(dirname "${output_file}")"

power_mode=unknown
if command -v nvpmodel >/dev/null 2>&1; then
  power_mode=$(nvpmodel -q 2>/dev/null | tr '\n' ';' || true)
fi

thermal_summary=unavailable
if compgen -G '/sys/class/thermal/thermal_zone*/temp' >/dev/null; then
  thermal_summary=$(for zone in /sys/class/thermal/thermal_zone*; do
    type=$(cat "${zone}/type" 2>/dev/null || echo unknown)
    temp=$(cat "${zone}/temp" 2>/dev/null || echo unknown)
    printf '%s=%s;' "${type}" "${temp}"
  done)
fi

cat >"${output_file}" <<EOF
schema_version=1
platform=linux-aarch64
timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)
uname=$(uname -a)
architecture=$(uname -m)
logical_cpu_count=$(getconf _NPROCESSORS_ONLN)
cmake_version=$(cmake --version | head -1)
git_version=$(git --version)
power_mode=${power_mode}
thermal_millidegrees_c=${thermal_summary}
EOF
