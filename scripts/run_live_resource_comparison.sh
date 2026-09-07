#!/usr/bin/env bash
set -euo pipefail
if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "usage: $0 <face_benchmark> <camera-index> <output-root> [presentation-config]" >&2; exit 2
fi
benchmark=$(realpath "$1"); camera=$2; output=$(realpath -m "$3"); config=${4:-}
[[ -x "$benchmark" ]] || { echo "benchmark is not executable: $benchmark" >&2; exit 2; }
[[ "$camera" =~ ^[0-9]+$ ]] || { echo "camera index must be non-negative" >&2; exit 2; }
if [[ -n "$config" ]]; then config=$(realpath "$config"); [[ -f "$config" ]] || { echo "configuration not found" >&2; exit 2; }; fi
mkdir -p "$output"
modes=(full); [[ -n "$config" ]] && modes+=(configured)
for mode in "${modes[@]}"; do
  for run in 1 2 3; do
    stem=$(printf '%s-run-%02d' "$mode" "$run")
    args=("--camera=$camera" --backend=yunet --warmup=30 --frames=900 --resource-profile --resource-sample-ms=200 "--output=$output/$stem.json" "--trace=$output/$stem-frames.csv" "--resource-trace=$output/$stem-resources.csv")
    [[ "$mode" == configured ]] && args+=("--presentation-config=$config")
    telemetry_pid=''
    if command -v tegrastats >/dev/null 2>&1; then tegrastats --interval 1000 --logfile "$output/$stem-tegrastats.log" >/dev/null 2>&1 & telemetry_pid=$!; fi
    "$benchmark" "${args[@]}"
    if [[ -n "$telemetry_pid" ]]; then kill "$telemetry_pid" 2>/dev/null || true; wait "$telemetry_pid" 2>/dev/null || true; fi
  done
done
(cd "$output" && sha256sum -- * > SHA256SUMS.txt)
echo "Live resource comparison completed: $output"
