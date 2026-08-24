# Face Project — Follow-on DMS Development Plan

## Status

- Baseline: `main` at `f135a4acda184a4f4ce8f58d2a9d689e6b9602f2`
- Completed predecessor: 16-step MediaPipe integration program
- Plan state: **ACCEPTED AND FROZEN**
- Plan accepted by the user: **2026-08-22**
- Current formal stage: **Stage 19 — YuNet + PFLD versus YuNet + LBF benchmark**
- Stage 17 status: **COMPLETE — implementation, sustained tests, and Windows/Orin camera validation passed**
- Stage 18 status: **COMPLETE — implementation and Windows/Ubuntu/Orin validation passed on 2026-08-24**
- Stage 19 status: **IN PROGRESS — approved by the user on 2026-08-24**
- Later stages in this document are architectural commitments or benchmark gates, not accepted implementations.

## Plan governance

This document is the accepted scope and acceptance contract for the follow-on DMS program. Implementation may not silently weaken, omit, or redefine a stage objective, architecture constraint, acceptance criterion, or required platform gate.

The plan may be amended when measurements, target limitations, dataset evidence, licensing, safety considerations, or product decisions require it. Every amendment must:

1. state the evidence or decision that requires the change;
2. identify the affected scope and acceptance criteria;
3. preserve completed-stage evidence;
4. be reviewed with the user when it materially changes product behavior, platform scope, safety/accuracy expectations, privacy, or required physical action; and
5. be committed before implementation proceeds under the amended contract.

Minor clarifications that do not change scope or acceptance may be committed with their implementation evidence.

## User-interaction legend

- **USER ACTION — CAMERA/DEVICE:** unavoidable physical camera or target-device access, or live GUI observation.
- **USER INPUT — DATA/POLICY/PRODUCT:** a dataset authorization, privacy/policy choice, event definition, class taxonomy, hardware fact, or material model-selection decision.
- **AUTONOMOUS:** inspection, implementation, builds, non-interactive Windows/SSH diagnostics, headless tests, packaging, and evidence recording performed by the agent.

The agent must complete all safe autonomous work before requesting a marked user action or input.

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
- A checksum-pinned external benchmark input and committed command produce structured results on both platforms with GUI disabled. The personal validation photograph must not be committed; its filename, dimensions, and SHA-256 identify the accepted bytes.
- Baseline and candidate results report stage timings plus p50/p95/p99 end-to-end latency, CPU, RSS, drops, resolution, build type, and platform metadata.
- The MediaPipe video candidate uses strictly monotonic timestamps and has focused tests for timestamp rejection/reset and result compatibility.
- Still-image backend tests and all existing CTests pass on Windows x64 and Orin aarch64.
- The candidate shows a material measured improvement on Orin, or the evidence identifies a different dominant bottleneck and the roadmap is updated before further optimization.
- Accuracy parity is checked on the benchmark sequence; no unexplained detection/landmark regressions are accepted.
- The application continues to have no direct FaceMediaPipe DLL/SO dependency and the bridge ABI compatibility policy is documented.
- A live-camera comparison is requested only after headless evidence passes; user action is limited to camera placement and visual confirmation.

### Required user involvement

- **USER ACTION — CAMERA/DEVICE:** after the headless gate passes, make the camera available on Windows and Orin (moving it between machines if shared) and visually confirm overlay quality, responsiveness, and clean exit. The agent prepares and runs all accessible commands.
- No user action is required for Stage 17 before this final live-camera gate.

### Implementation and validation evidence (2026-08-22 through 2026-08-24)

- Added a Release/headless `face_benchmark` path with structured JSON output, warm-up and measured-frame control, throughput, CPU-capacity percentage, RSS, and capture/backend/semantic/end-to-end latency distributions.
- Changed the private MediaPipe bridge from `RunningMode::IMAGE`/`Detect()` to timestamped `RunningMode::VIDEO`/`DetectForVideo()` while preserving the five-function public C ABI. Strictly monotonic handle-local timestamps and focused conversion/result tests pass.
- Reused bridge normalized-landmark storage, application landmark storage, and `FaceResult` landmark capacity to remove avoidable per-frame container allocation.
- Fresh Windows and Orin bridge builds, exact-export/package checks, three focused bridge tests, clean Release CMake builds, and all ten application CTests passed. The common validation image produced 100/100 successful detections in every measured run.
- Windows headless MediaPipe throughput improved from the IMAGE baseline of 34.805 FPS to 48.795, 54.831, and 53.386 FPS in three VIDEO runs (mean 52.337 FPS, approximately 50% higher).
- Orin headless MediaPipe throughput improved from the IMAGE baseline of 3.891 FPS to 5.417, 5.193, and 5.359 FPS in three VIDEO runs (mean 5.323 FPS, approximately 37% higher). Mean backend latency fell from 252.991 ms to approximately 184.156 ms. Orin remained in `MAXN_SUPER`, near 48.5 C, without observed thermal throttling; MediaPipe selected XNNPACK CPU and GPU utilization remained zero.
- Live Windows camera validation used MSMF at 640x480/30 FPS. YuNet/LBF measured 18.81 mean FPS and 33.64 ms mean backend time; MediaPipe measured 29.49 mean FPS and 13.70 ms mean backend time. Both tracked responsively and exited cleanly without runtime errors.
- Live Orin camera validation used V4L2 at 640x480/30 FPS. YuNet/LBF measured 29.75 mean FPS and 25.47 ms mean backend time. MediaPipe measured 5.65 median FPS (6.33 mean distorted by brief fast/no-face intervals), 168.81 ms median backend time, and visibly delayed/jerky output. This agrees with the deterministic Orin result and identifies CPU inference as the dominant remaining bottleneck.
- User-observed blink behavior is not production-ready and is recorded as algorithm evidence rather than a Stage 17 performance regression. For five deliberate blinks, Windows YuNet/LBF reported roughly 10-15 and Windows MediaPipe 8-10, with head-motion false positives. Orin YuNet/LBF reported roughly 2-3 and Orin MediaPipe 1-2; the slow MediaPipe sampling missed eye closures. The fixed EAR threshold and transition-only counter lack calibration, hysteresis, duration, pose/visibility gating, and the planned temporal FSM.
- The live-camera user-action gate is complete on Windows and Orin. No further camera action is required for Stage 17.
- The user explicitly confirmed on 2026-08-24 that the personal benchmark photograph must not be committed. The acceptance wording was amended from a committed input to a checksum-pinned external input without changing the tested bytes or reproducibility contract.
- Sustained 1,000-frame MediaPipe runs retained 1,000/1,000 detections. Windows achieved 60.568 FPS with approximately -53 KB sampled post-warm-up working-set change. Orin achieved 5.519 FPS; sampled RSS changed from approximately 214,496 KB after warm-up to 214,600 KB at completion (approximately 104 KB), showing bounded memory rather than continuing growth.
- Benchmark schema version 2 records build configuration, decoded dimensions, synchronous dropped/superseded/rendered counts, and initial/final/peak RSS in addition to the accepted latency distributions. Schema sanity runs passed on Windows and Orin Release builds.
- The runtime benchmark deliberately reports the stable public boundary as aggregate `backend` time: bridge-side BGR-to-RGB conversion, MediaPipe inference, and result conversion remain included rather than exposed by changing the five-function C ABI. XNNPACK selection, zero Orin GPU activity, consistent deterministic/live latency, and the VIDEO-mode delta identify CPU inference as the dominant Orin cost. A future compatible private diagnostic may split that aggregate, but it is not required before scheduling/model comparison work.

### Stage 17 conclusion so far

VIDEO mode is a material optimization on both targets, but full-frame MediaPipe Face Landmarker remains CPU-bound and is not a viable every-frame Orin production path at 640x480. MediaPipe remains useful as an accuracy/reference candidate and may be scheduled at a lower cadence; Stage 18's bounded latest-frame scheduler and Stage 19's controlled lightweight-geometry benchmark remain necessary. YuNet/LBF remains the real-time baseline, not the selected production landmark stack.

Stage 17 is complete. Its result does not select a production landmark stack: it establishes that MediaPipe VIDEO mode is materially better, that every-frame MediaPipe is still unsuitable on Orin CPU, and that bounded scheduling plus controlled lightweight-model comparison must precede production selection.

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

### Required user involvement

- None expected. Any new material scheduling/product tradeoff discovered by measurement is handled through the plan-amendment procedure.

### Implementation evidence (complete)

- Added an OpenCV- and provider-independent `dms_core` CMake target with semantic observation headers and values for eye geometry, face geometry, driver presence, recognition and associated objects.
- Every observation carries source-frame identity, monotonic capture/production timing, age at production, explicit missing/valid/occluded state, confidence and visibility. A single policy classifies future, stale and low-quality observations without treating them as negative events.
- Added independently configured eye, geometry, recognition and object cadences with maximum ages, quality thresholds and optional uncertainty-triggered execution. Scheduling consumes caller-supplied monotonic time and rejects non-monotonic updates.
- Added a thread-safe depth-one latest-frame slot with published, consumed and superseded counters. Slow work replaces pending input rather than creating an unbounded queue.
- A deterministic recorded-timestamp test verifies independent task counts, all quality states, configuration rejection and depth-one supersession without sleeping or reading wall-clock time.
- Fresh `D:\work\p18` Windows x64 and `~/common/p18` Orin aarch64 Release configure/builds passed at commit `c65355a`; all eleven CTests passed on both targets. A source isolation audit found none of the prohibited provider/runtime technology names or facial landmark topology indices in DMS core code.
- The final portability pass found and corrected a CMake scope defect that linked `face_benchmark` to OpenCV only when the optional MediaPipe runtime was enabled. Windows and Orin Release rebuilds with the correction passed all eleven CTests.
- Native x64 Ubuntu validation used a new Ubuntu 24.04.4 LTS WSL distro, GCC 13.3.0, CMake 3.28.3, and a locally built minimal OpenCV 4.8.0 plus contrib installation matching the accepted project ABI. The fresh `/root/p18/build-ubuntu-opencv48` Release build passed all ten applicable CTests, including both real-image YuNet/LBF integration tests. The MediaPipe package-specific integration test is not registered because no x64 Ubuntu MediaPipe runtime package is configured.
- The previous Ubuntu 23.10 WSL distro was preserved rather than destructively upgraded. Its pre-upgrade export remains external to the repository at `D:\work\p18\ubuntu-23.10-pre-upgrade.tar`; no personal validation photograph was committed.

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

### Required user involvement

- **USER INPUT — DATA/POLICY/PRODUCT:** authorize access to representative visible/IR DMS recordings and state any privacy or retention restrictions. If no suitable dataset exists, review the proposed recording and annotation protocol before collection.
- **USER INPUT — DATA/POLICY/PRODUCT:** review the completed benchmark and approve the production facial-geometry direction: PFLD-class provider, optimized MediaPipe, temporary LBF retention, or a specialized eye-model evaluation.

### Implementation evidence (in progress)

- Stage 19 was authorized by the user on 2026-08-24.
- Added `docs/STAGE19_MODEL_PROVENANCE.md` with the pinned 68-point PFLD candidate's source revisions, SHA-256, topology, preprocessing, MIT attribution, and explicit undocumented-training-lineage limitation. No fetched model or research repository is committed.
- Added `docs/STAGE19_DATASET_PROTOCOL.md` with the privacy boundary, required visible/IR and robustness slices, annotation contract, frozen provider comparison, accuracy/resource metrics, and product-weighted selection rule.
- The user confirmed on 2026-08-24 that no suitable recordings currently exist, temporary frame extraction will be allowed when recordings are supplied, no special retention restriction currently applies, and anonymous aggregate metrics and recording checksums may be committed. Raw recordings, frames and biometric annotations remain outside Git.
- Added `docs/STAGE19_RECORDING_GUIDE.md` with a safe single-subject bring-up session and the multi-subject, intended-camera expansion required before production selection.
- Added a pure-C++ OpenCV DNN PFLD provider for the pinned external 68-point ONNX candidate. Its preprocessing and output conversion remain inside the provider, and a separate candidate-specific mapper is the only new code containing its iBUG topology indices. CMake verifies the external model's recorded SHA-256 before registering the real-image integration test; neither the model nor the personal test image is committed.
- Fresh revision `8949cbd` Release builds passed all 12 applicable CTests, including checksum-pinned real PFLD inference and semantic-eye mapping, on Windows x64 at `D:\work\p19`, native x64 Ubuntu 24.04 at `/root/p19`, and Orin aarch64 at `~/common/p19`. Orin test binaries were confirmed as ARM aarch64 with no unresolved dependency. The target recording benchmark and provider-selection decision remain pending the authorized recordings.

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

### Required user involvement

- **USER INPUT — DATA/POLICY/PRODUCT:** approve operational definitions after recommended defaults and tradeoffs are presented, including PERCLOS window, prolonged-closure duration, yawn duration, distraction duration, pose zones, and whether each event is displayed, counted, recorded, or alerted.
- No user input is required while implementing and testing the model-neutral metric/FSM mechanisms before that definition gate.

## Stage 21 — recognition and object/context events

### Objective

Add consented driver recognition plus cigarette-at-mouth and hand-held-object/drink events as separate components.

### Acceptance gate

- Recognition uses face detection/alignment, embeddings, enrollment, matching thresholds, unknown-driver behavior, and privacy/retention controls; it is not landmark identity matching.
- Cigarette and object events use an object/context detector with face/mouth/hand spatial association and temporal confirmation.
- “Anything else in hand” is narrowed to an annotated class/taxonomy and measurable unknown-object policy before implementation.
- Accuracy, subgroup/condition slices, latency, and false alarms per hour pass agreed dataset gates.

### Required user involvement

- **USER INPUT — DATA/POLICY/PRODUCT:** approve recognition enrollment, expected-driver/unknown-driver behavior, acceptable false acceptance/rejection, and image/embedding retention and deletion policy.
- **USER INPUT — DATA/POLICY/PRODUCT:** approve the initial hand-held-object taxonomy and unknown-object behavior.
- **USER INPUT — DATA/POLICY/PRODUCT:** after pretrained baselines are reported, approve any proposed data annotation, training, or fine-tuning effort. Production inference remains pure C++ even if offline training uses Python.

## Stage 22 — native packaging matrix and Raspberry Pi enablement

### Objective

Produce self-contained CMake application packages for Windows x64, x64 Ubuntu, Orin aarch64, and Raspberry Pi 5 aarch64, with target-specific model/runtime payloads behind common interfaces.

### Acceptance gate

- CI or documented clean builds cover all four targets; no target downloads dependencies at application startup.
- Architecture, dependencies, model checksums, licenses, manifests, and launcher behavior are verified.
- Raspberry Pi passes CPU baseline tests first. Hailo is an optional, separately packaged execution provider and is accepted only after model conversion, accuracy parity, device FPS/latency, thermals, and sustained-run tests pass.
- At least a two-hour sustained run demonstrates bounded memory/latency and reports thermals, power mode, and dropped frames on each embedded target.

### Required user involvement

- **USER ACTION — CAMERA/DEVICE:** when available, provide Raspberry Pi network/device access and physically connect the intended camera. Do not provide or expose credentials in repository data.
- **USER INPUT — DATA/POLICY/PRODUCT:** confirm whether a Hailo accelerator is present and its exact model before the optional Hailo path begins.
- **USER ACTION — CAMERA/DEVICE:** at the final gate, make the camera available on each physical target and visually confirm live behavior. The agent performs all builds, commands, packaging, and non-interactive validation.

## Formal review checkpoints

1. **Completed — plan freeze:** overall scope, architecture and acceptance criteria accepted on 2026-08-22.
2. **After Stage 17:** review whether optimized MediaPipe remains a serious production candidate.
3. **After Stage 19:** select the production facial-geometry direction.
4. **During Stage 20:** approve operational event definitions and thresholds.
5. **Before Stage 21 product integration:** approve recognition/privacy policy and object taxonomy.
6. **When Raspberry Pi is available:** confirm hardware, access and optional accelerator scope.

## Current recommendation

Do not select YuNet + LBF as the final production stack now. Retain it as the low-complexity baseline. Benchmark YuNet + a PFLD-class ONNX model as the leading lightweight candidate, MediaPipe as the current accuracy/reference candidate, and a dedicated eye ROI model as a conditional candidate for eye-specific accuracy.

The next implementation work is Stage 17. It is logically prior to adding PFLD or DMS features because it creates the benchmark harness, timestamps, latency accounting, and scheduling evidence required to compare every later model fairly.

## Inputs currently unavailable

The requested `orin.txt` and `windows.txt` console logs were not present in the repository or supplied workspace when this plan was created. Stage 17 can begin from source instrumentation, but the logs should be added as non-secret validation inputs when available so their original runs can be correlated with the new measurements.
