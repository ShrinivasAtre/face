# Face Project — Follow-on DMS Development Plan

## Status

- Baseline: `main` at `f135a4acda184a4f4ce8f58d2a9d689e6b9602f2`
- Completed predecessor: 16-step MediaPipe integration program
- Current formal stage: **Stage 17 — performance characterization and MediaPipe video-path optimization**
- Stage 17 status: **PLANNED**
- Later stages in this document are architectural commitments or benchmark gates, not accepted implementations.

## Product objective

Build a pure C++17 driver monitoring application that is CMake-built and deployable as a self-contained application on Windows x64, NVIDIA Jetson Orin/Linux aarch64, x64 Ubuntu, and Raspberry Pi 5/Linux aarch64. Preserve the existing runtime-loaded MediaPipe boundary while allowing independently packaged detector, landmark, recognition, eye, object, and accelerator implementations.

The system is intended to observe and temporally classify:

- driver presence and identity;
- eye openness, EAR, PERCLOS, blinks, and prolonged closure/drowsiness;
- mouth opening and yawns;
- head pose, left/right and up/down movement counts;
- gaze and visual distraction;
- face/eye/mouth occlusion;
- cigarette-at-mouth events;
- hand-held objects and drink-container events;
- quality/confidence and finite-state-machine state.

This is not a safety-certified system. Production feasibility must be decided from the target IR-camera dataset and target-device evidence, including false-alarm and missed-event costs.

## Architecture constraints

1. CMake remains the application build system. Bazel remains confined to the existing MediaPipe bridge.
2. Runtime code is pure C++; Python may be used only for offline training, export, conversion, or dataset tooling.
3. DMS algorithms consume backend-neutral semantic observations, not YuNet, LBF, MediaPipe, PFLD, or any other model's raw landmark indices.
4. Topology-specific adapters alone may know raw landmark indices. They publish semantic eye contours, mouth contours, stable pose points, confidence, visibility, and observation timestamps.
5. Model inference is scheduled by task cadence and observation quality. Eye refinement, object detection, and recognition are not required on every camera frame.
6. Every observation carries source-frame ID, monotonic timestamp, age, validity, and confidence. Temporal logic must reject stale observations.
7. Temporal filters and event FSMs are separate from inference backends. Thresholds and debounce durations are configuration, not compiled topology assumptions.
8. Face recognition is a separate embedding/matching component; cigarette and hand-held-object detection are separate object/context models. Neither is inferred from facial landmarks alone.
9. Acceleration is selected behind runtime interfaces. Candidate execution paths are ONNX Runtime CPU on all targets, TensorRT/CUDA on Orin, Windows CPU/CUDA/DirectML as supported by the packaged runtime, and CPU initially on Raspberry Pi with a separately benchmarked Hailo path if hardware is present.
10. Raspberry Pi support must not be claimed until native-device build, package, thermal, and accuracy gates pass.

## Proposed runtime pipeline

Each captured frame enters a bounded latest-frame pipeline. Slow consumers may skip superseded frames rather than create unbounded latency.

1. Capture and normalize timestamps.
2. Run or track the face ROI.
3. Produce backend-neutral facial geometry on its configured cadence.
4. Run an optional eye ROI refinement model only when scheduled or when landmark quality is insufficient.
5. Run recognition and object/context models at independent lower cadences.
6. Update semantic metrics (eye openness, EAR, mouth openness, pose, gaze, visibility).
7. Apply temporal filtering.
8. Update event FSMs and counters.
9. Render the most recent non-stale state and emit structured telemetry.

No UI, capture, or slow inference queue may determine event time. Event time comes from the source-frame monotonic timestamp.

## Stage 17 — performance characterization and MediaPipe video path

### Objective

Explain the Windows/Orin FPS difference with reproducible evidence, then reduce MediaPipe latency and CPU/allocation overhead without changing the accepted backend-neutral result contract or DMS behavior.

### Scope

- Add machine-readable per-stage timing for capture, color conversion, MediaPipe inference, result conversion, shared blink processing, rendering, and end-to-end latency.
- Record FPS, p50/p95/p99 latency, CPU utilization, memory growth, dropped/superseded frames, input resolution/format, build type, platform power mode, and thermal/throttling state.
- Add a deterministic video/file benchmark so Windows and Orin process identical frames without camera or GUI variability.
- Compare the current MediaPipe `IMAGE`/`Detect()` path with the timestamped `VIDEO` path.
- Measure and, where evidence supports it, remove avoidable per-frame allocations through handle-owned reusable storage.
- Evaluate latest-frame scheduling and configurable landmark cadence; do not change event semantics in this stage.
- Preserve `--backend=yunet` and `--backend=mediapipe`, the runtime-loaded bridge, the existing five C ABI exports, and current still-image results.

### Known hypotheses to test

- The bridge currently selects MediaPipe `RunningMode::IMAGE`, so it performs full image inference rather than using the video-mode tracking behavior.
- RGB `ImageFrame`, normalized-landmark storage, bridge landmark storage, and application landmark storage are allocated or rebuilt on every frame.
- Synchronous capture, inference, rendering, and console output share one loop, so camera blocking or GUI work can cap measured FPS and hide inference throughput.
- Orin power/clock mode, thermal throttling, camera pixel format, or a non-Release build may contribute independently of model inference.

### Acceptance gate

- Windows uses a fresh `D:\work\p17` validation root and Orin uses a fresh `~/common/p17` validation root.
- A committed benchmark input and command produce structured results on both platforms with GUI disabled.
- Baseline and candidate results report stage timings plus p50/p95/p99 end-to-end latency, CPU, RSS, drops, resolution, build type, and platform metadata.
- The MediaPipe video candidate uses strictly monotonic timestamps and has focused tests for timestamp rejection/reset and result compatibility.
- Still-image backend tests and all existing CTests pass on Windows x64 and Orin aarch64.
- The candidate shows a material measured improvement on Orin, or the evidence identifies a different dominant bottleneck and the roadmap is updated before further optimization.
- Accuracy parity is checked on the benchmark sequence; no unexplained detection/landmark regressions are accepted.
- The application continues to have no direct FaceMediaPipe DLL/SO dependency and the bridge ABI compatibility policy is documented.
- A live-camera comparison is requested only after headless evidence passes; user action is limited to camera placement and visual confirmation.

## Stage 18 — backend-neutral DMS observation and scheduling core

### Objective

Introduce semantic observation packets, bounded latest-frame scheduling, independent task cadences, configuration, and deterministic time-based tests without adding new production models.

### Acceptance gate

- DMS metric and FSM code contains no backend topology indices and includes no MediaPipe, LBF, PFLD, ONNX Runtime, TensorRT, DirectML, or Hailo headers.
- Eye, geometry, recognition, and object workers can run at different configured cadences.
- Stale, missing, low-confidence, and occluded inputs have explicit behavior.
- Queue depth is bounded and latency does not grow when an inference worker is slower than capture.
- Recorded-sequence tests are deterministic and independent of wall-clock scheduling.
- Windows x64, x64 Ubuntu, and Orin aarch64 CMake builds and tests pass. Raspberry Pi remains an architecture review until hardware is available.

## Stage 19 — YuNet + PFLD versus YuNet + LBF benchmark

### Objective

Add an ONNX landmark-provider plugin and compare at least one reproducibly sourced PFLD-class model with LBF and the accepted MediaPipe backend on the actual visible/IR DMS dataset. This stage is a selection benchmark, not a pre-decided migration.

### Required metrics

- normalized landmark error, failure rate, and jitter;
- eye-opening error and open/closed ROC/precision-recall;
- blink event precision/recall/F1 and count error;
- PERCLOS absolute error over fixed windows;
- yawning event precision/recall/F1;
- head-pose angular error and movement-count error;
- robustness slices for glasses, sunglasses, IR illumination, darkness, pose, distance, partial face, and occlusion;
- FPS, p50/p95/p99 latency, CPU/GPU utilization, memory, power, and thermal stability on each available target.

### Acceptance gate

- Model provenance, license, topology, preprocessing, checksum, and training data limitations are recorded.
- All topology is isolated in provider adapters and the same downstream DMS algorithms evaluate every provider.
- PFLD replaces LBF only if it wins the product-weighted accuracy/latency gate on the target dataset and does not make a required platform impractical.
- If generic facial landmarks do not meet eye-opening/PERCLOS targets, Stage 20 evaluates a dedicated eye ROI model rather than embedding provider-specific exceptions in DMS logic.

## Stage 20 — DMS metrics and temporal FSMs

### Objective

Implement calibrated eye openness, EAR, PERCLOS, blink, yawn, head pose, gaze, distraction, driver presence, occlusion, temporal filtering, and event FSMs using semantic observations.

### Rules

- Use time durations, not frame counts, for temporal thresholds.
- Treat drowsiness as a temporal decision derived from prolonged closure/PERCLOS/yawn and quality state, not as a single-frame landmark label.
- Distinguish unknown/occluded from open or closed.
- Count pose movements only on hysteretic state transitions with return-to-neutral rules.
- Permit an eye ROI model to run periodically, on uncertainty, after reacquisition, and around suspected transitions rather than every frame.

### Acceptance gate

- Each event has a written definition, ground-truth annotation rule, confidence policy, debounce/hysteresis policy, and deterministic unit/sequence tests.
- Dataset-level event precision, recall, F1, false alarms per hour, count error, and detection delay are reported by required robustness slice.
- Threshold calibration data is separate from final evaluation data.

## Stage 21 — recognition and object/context events

### Objective

Add consented driver recognition plus cigarette-at-mouth and hand-held-object/drink events as separate components.

### Acceptance gate

- Recognition uses face detection/alignment, embeddings, enrollment, matching thresholds, unknown-driver behavior, and privacy/retention controls; it is not landmark identity matching.
- Cigarette and object events use an object/context detector with face/mouth/hand spatial association and temporal confirmation.
- “Anything else in hand” is narrowed to an annotated class/taxonomy and measurable unknown-object policy before implementation.
- Accuracy, subgroup/condition slices, latency, and false alarms per hour pass agreed dataset gates.

## Stage 22 — native packaging matrix and Raspberry Pi enablement

### Objective

Produce self-contained CMake application packages for Windows x64, x64 Ubuntu, Orin aarch64, and Raspberry Pi 5 aarch64, with target-specific model/runtime payloads behind common interfaces.

### Acceptance gate

- CI or documented clean builds cover all four targets; no target downloads dependencies at application startup.
- Architecture, dependencies, model checksums, licenses, manifests, and launcher behavior are verified.
- Raspberry Pi passes CPU baseline tests first. Hailo is an optional, separately packaged execution provider and is accepted only after model conversion, accuracy parity, device FPS/latency, thermals, and sustained-run tests pass.
- At least a two-hour sustained run demonstrates bounded memory/latency and reports thermals, power mode, and dropped frames on each embedded target.

## Current recommendation

Do not select YuNet + LBF as the final production stack now. Retain it as the low-complexity baseline. Benchmark YuNet + a PFLD-class ONNX model as the leading lightweight candidate, MediaPipe as the current accuracy/reference candidate, and a dedicated eye ROI model as a conditional candidate for eye-specific accuracy.

The next implementation work is Stage 17. It is logically prior to adding PFLD or DMS features because it creates the benchmark harness, timestamps, latency accounting, and scheduling evidence required to compare every later model fairly.

## Inputs currently unavailable

The requested `orin.txt` and `windows.txt` console logs were not present in the repository or supplied workspace when this plan was created. Stage 17 can begin from source instrumentation, but the logs should be added as non-secret validation inputs when available so their original runs can be correlated with the new measurements.
