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
2. Add an external sampler for per-core CPU, process threads, temperature and
   throttling without perturbing inference timing.
3. Add explicit benchmark modes for R1, R4, R5, R6 and R7 comparisons.
4. Run identical recorded input on Windows x64, Ubuntu x64 and Orin aarch64.
5. Run camera scenarios only after notifying the user that physical camera
   availability is required.
6. Define any CPU/cadence budget only after evidence review; do not silently
   make a measured maximum into a product requirement.

