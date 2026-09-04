# CPU, core and memory benchmark plan

Date: 2026-09-03

## Objective

Measure how much CPU capacity, which logical cores, and how much resident memory
the application consumes overall and during distinct DMS activities. This
characterizes behavior; it does not change production thresholds or declare
that a utilization ceiling is safe.

## Scenarios

| ID | Scenario | Input and state | Required measurements |
|---|---|---|---|
| R0 | Idle/startup | Initialized process before frame processing | startup time, RSS/working set, thread count |
| R1 | Capture only | Fixed camera/video format; providers disabled by benchmark mode | capture latency, total CPU, per-core CPU, RSS |
| R2 | Calibration | Stable forward/open-eye sequence through calibration completion | calibration duration, per-stage latency, total/per-core CPU, RSS |
| R3 | Monitoring | Fixed checksum-pinned recorded stream | throughput, drops, capture/provider/semantic/render latency, total/per-core CPU, RSS |
| R4 | Face detection | Detector cadence isolated | detector latency distribution and resource deltas |
| R5 | Landmark/eye | Geometry and eye-quality path isolated | provider, mapping and eye-metric latency/resource deltas |
| R6 | Blink/yawn/FSM | Deterministic observations without model inference | semantic/FSM latency and resource floor |
| R7 | Display | Same R3 input with GUI off versus on | render latency and CPU/RSS delta |
| R8 | Sustained | At least 30 minutes on each target | peak/growth RSS, per-core CPU, temperature, throttling and drops |

## Machine-readable result contract

Each run records:

- schema version, Git revision, dirty state and policy profile;
- platform, OS/BSP, CPU model, physical/logical core counts and affinity mask;
- compiler/build type, OpenCV version, provider/model identifiers and hashes;
- input checksum, resolution, pixel format, nominal FPS and measured frames;
- wall time, process CPU time and percentage of total machine capacity;
- per-logical-core mean, p95 and maximum utilization;
- initial/final/peak RSS or working set and growth;
- capture, provider, semantic/FSM, render and end-to-end p50/p95/p99 latency;
- published, consumed, superseded, dropped and rendered frame counts;
- target power mode, temperature and throttle state where available.

Private media paths are never written to committed results. Only anonymous input
IDs and checksums may be retained under the approved data policy.

## CPU selection and limiting policy

Affinity and utilization limiting are different controls:

- **Affinity** restricts eligible logical CPUs. It can reduce interference but
  does not guarantee a CPU percentage.
- **Cadence** controls how often each inference task runs and is the preferred
  application-level resource control.
- **Thread-count/runtime controls** may constrain OpenCV or inference providers,
  but must be set explicitly and benchmarked per platform.
- **OS scheduling/containers** can enforce harder quotas outside the
  application. Windows job objects, Linux cgroups/systemd, and target BSP
  controls remain deployment adapters.

The application may expose an optional affinity mask only after invalid masks,
processor groups, heterogeneous cores, inheritance, and packaging behavior are
tested. The default remains unrestricted OS scheduling.

## Acceptance sequence

1. Reuse the existing face_benchmark JSON timing and overall CPU/RSS fields.
2. Add an optional in-process sampler for portable process/per-core CPU,
   process threads and memory, then correlate target-specific temperature and
   throttling from external BSP tools without perturbing inference timing.
3. Add explicit benchmark modes for R1, R4, R5, R6 and R7 comparisons.
4. Run identical recorded input on Windows x64, Ubuntu x64 and Orin aarch64.
5. Run camera scenarios only after notifying the user that physical camera
   availability is required.
6. Define any CPU/cadence budget only after evidence review; do not silently
   make a measured maximum into a product requirement.

## Implementation checkpoint — 2026-09-03

Steps 1--4 are implemented on `feature/stage23-resource-instrumentation` as
schema 6.
The benchmark now emits source/platform/compiler metadata, periodic process and
per-core CPU samples, resident/private memory, thread count, operational phase
summaries and component latency distributions. Windows uses native process
counters and logical-processor performance information; Linux uses `getrusage`
and `/proc`. Raw samples are optional CSV output.

A Windows Release smoke run on a private recorded clip produced parseable JSON,
18 matching CSV/JSON samples, and eight logical-core values per sample. This is
implementation evidence only, not a performance baseline. Repeated profiler
overhead measurement, Ubuntu validation and target-specific thermal collection
remain open.

Orin aarch64 validation at commit `f5cc99a` used a fresh
`~/common/p23/face-stage23` checkout, the accepted Stage 17 MediaPipe runtime,
GCC 13.3.0 and OpenCV 4.8.0. The Release build passed all 22 applicable CTests,
including `resource_profiler_test`. A 20-frame checksum-pinned still-image
smoke run detected all 20 frames and produced schema-6 output with 186 matching
resource samples across all six logical cores. The observed 5.507 FPS and
resource values are smoke evidence only, not a repeated benchmark baseline.
Profiler overhead, recorded-video phase coverage, thermals and sustained-run
evidence remain open.

### Profiling overhead checkpoint — 2026-09-04

The sampler thread now runs below normal inference priority on Windows and at
Linux nice level 10. The change is commit `db43445` and is pushed to the GitHub
Stage 23 branch.

- Windows used three interleaved profiled/unprofiled Release runs of 500
  measured frames on the checksum-pinned image. All six runs detected 500/500
  frames. Median throughput was 53.35 FPS without sampling and 57.06 FPS with
  200 ms sampling; median end-to-end p95 was 23.66 ms and 21.11 ms,
  respectively. The apparent profiled speedup is treated as environmental run
  variance, not a performance benefit; no Windows slowdown was measurable.
- Orin used three runs per mode of 100 measured frames. Before lowering sampler
  priority, median 200 ms sampled throughput was 5.20 FPS versus 5.43 FPS
  unsampled (about -4.2%). After lowering priority it was 5.31 FPS (about
  -2.2%), while median end-to-end p95 changed from 213.28 ms to 213.70 ms
  (about +0.2%). All runs detected 100/100 frames with unchanged event output.
- The profiler remains explicitly enabled and is off by default. These results
  accept it for diagnostic characterization but do not make it suitable for
  permanent production telemetry. Longer Orin runs and target thermal
  correlation remain part of the sustained gate.

### Calibration/processing and thermal checkpoint — 2026-09-04

A revision-correct Orin run at `db43445` processed 500 measured frames after 20
warm-up frames with 500/500 detections. The 200 ms sampler produced 499 samples:
62 during initial calibration and 415 during steady processing. Throughput was
5.205 FPS. Steady-processing RSS ranged from 227,414,016 to 227,487,744 bytes,
a 73,728-byte range; the larger startup-to-final increase is model/runtime
initialization and is not treated as a leak measurement.

The Orin remained in `MAXN_SUPER`. A 133-sample `tegrastats` capture reported a
maximum junction temperature of 48.531 C, maximum observed VDD_IN of 4,360 mW,
and zero GR3D utilization, consistent with XNNPACK CPU inference. No thermal
concern was observed in this short run. This closes initial calibration and
normal-processing phase coverage on Windows and Orin; recalibration and the
30-minute sustained gate remain open.
