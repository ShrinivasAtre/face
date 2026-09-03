# Face Project — Follow-on DMS Development Plan

## Status

- Baseline: `main` at `f135a4acda184a4f4ce8f58d2a9d689e6b9602f2`
- Completed predecessor: 16-step MediaPipe integration program
- Plan state: **ACCEPTED AND FROZEN**
- Plan accepted by the user: **2026-08-22**
- Current formal stages: **Stage 19 dataset expansion and Stage 20 accuracy acceptance**
- Stage 17 status: **COMPLETE — implementation, sustained tests, and Windows/Orin camera validation passed**
- Stage 18 status: **COMPLETE — implementation and Windows/Ubuntu/Orin validation passed on 2026-08-24**
- Stage 19 status: **IN PROGRESS — approved by the user on 2026-08-24**
- Stage 20 status: **IN PROGRESS — mechanisms, approved policy, first dense scoring, and second-batch tooling complete; new recordings and production-accuracy evidence remain open**
- Sponsor recorded-video demonstration: **IMPLEMENTED AND PACKAGED — final physical rehearsal pending**
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
- Added YuNet+PFLD to the same headless benchmark used by LBF and MediaPipe, plus private per-frame CSV tracing and deterministic image-sequence input. Windows and MediaPipe-enabled Orin builds pass all 13 CTests. Raw clips, decoded frames and traces remain external to Git.
- Added `docs/STAGE19_PRELIMINARY_RESULTS.md`. On the one-subject visible-light development set, all providers had 100% availability in the measured clips. PFLD was only modestly faster than LBF on Orin and slower on the short Windows natural sample; MediaPipe was fastest on Windows but slowest on Orin. For ten narrated deliberate blinks, the unchanged diagnostic counter reported LBF/PFLD/MediaPipe counts of 172/30/12. PFLD therefore does not pass the replacement gate. Frame-accurate accuracy metrics, missing production slices and user review remain outstanding, so Stage 19 stays in progress.
- Final regression at results commit `fa544fe` passed all 13 MediaPipe-enabled CTests on Windows x64 and Orin aarch64 and all 12 applicable CTests on native x64 Ubuntu. The Orin benchmark executable was confirmed as ARM aarch64 with no unresolved dependency.
- The user approved the preliminary direction on 2026-08-26: do not adopt the evaluated PFLD candidate; proceed to Stage 20 with MediaPipe as the current eye-geometry reference, retain LBF/PFLD as benchmark baselines, control MediaPipe cadence on Orin, and evaluate a dedicated eye ROI model only if calibrated provider-neutral logic misses the eye gates. Stage 19 will be rerun when broader production-representative clips arrive.

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

### Implementation evidence (in progress)

- Stage 20 implementation was explicitly authorized on 2026-08-27.
- Added provider-neutral `EyeCalibration` and `EyeTemporalMetrics` components to `dms_core`. Raw provider EAR values are normalized through an explicit closed/open calibration; hysteretic openness classification distinguishes open, closed and unknown without provider topology.
- Blink confirmation uses monotonic durations for minimum closure, maximum blink closure and confirmed reopening. Prolonged closure is duration based. Missing, stale, low-confidence and occluded observations become unknown, reset transient events and cannot create a blink.
- Rolling PERCLOS integrates closed time only over known-quality intervals, excludes excessive sample gaps, and remains unavailable until its configured known-time coverage is met. Non-monotonic samples are rejected without mutating state.
- Deterministic tests cover calibration, hysteresis, blink debounce/single counting, prolonged closure, occlusion, rolling PERCLOS, reset, non-monotonic timestamps and invalid configuration without sleeping or reading wall-clock time.
- Fresh revision `337fa19` Release validation passed all 14 MediaPipe-enabled CTests on Windows x64 at `D:\work\p20` and Orin aarch64 at `~/common/p20`, plus all 13 applicable CTests on native x64 Ubuntu 24.04 at `/root/p20`. No camera or user action was required.
- Recorded-input integration is in progress. The benchmark now repairs duplicate,
  missing, backward and loop-reset decoder timestamps onto a strictly monotonic
  nominal-cadence timeline and emits the provider-neutral eye FSM state, openness,
  blink event/count, closure duration, prolonged closure and PERCLOS coverage in
  its private trace and aggregate JSON.
- Initial second-subject Windows development runs used private neutral, blink,
  closure, partial-opening and occlusion clips. In a near-complete 1,300-frame
  pass of the narrated ten-blink clip, MediaPipe plus the duration FSM counted
  exactly ten confirmed blinks with no prolonged-closure frames; YuNet/LBF
  counted zero with the same initial calibration. In the corresponding closure
  pass, MediaPipe reported 127 prolonged-closure frames while YuNet/LBF reported
  none. The private distributions explain the difference: MediaPipe closure EAR
  reached approximately 0.03--0.05 while LBF remained above approximately 0.21,
  overlapping its open-eye scale. This is development evidence for explicit
  calibration and provider selection, not production accuracy.
  Both providers continued to report valid geometry during sampled occlusion,
  and MediaPipe produced two apparent blink transitions. This confirms that
  landmark availability is not an occlusion-confidence signal. The core already
  handles explicit unknown/occluded observations safely, but a separate
  provider-neutral eye-quality observation remains required before the occlusion
  gate can pass. Raw clips, audio, frames and traces remain external to Git.
- Recorded-input implementation commit `072e597` passed clean Release validation
  with all 15 configured tests on Windows x64 at
  `D:\work\p20\build-recorded-windows`, all 15 on Orin aarch64 at
  `~/common/p20/build-recorded-orin`, and all 14 applicable tests on native
  Ubuntu 24.04 x64 at `/root/p20/build-recorded-ubuntu`. The Windows and Orin
  suites included the real MediaPipe and PFLD still-image integrations; Ubuntu's
  accepted configuration keeps the optional MediaPipe runtime disabled.
- The same private 300-frame second-subject blink sequence produced four
  confirmed MediaPipe blinks and zero YuNet/LBF blinks on both Windows and Orin.
  On Orin's deterministic lossless image sequence, MediaPipe backend mean latency
  was 167.44 ms (5.56 end-to-end FPS), while YuNet/LBF was 15.59 ms (44.54 FPS).
  Direct Orin decoding again rejected the phone MP4 through GStreamer, so the
  established temporary lossless image-sequence path was used and deleted after
  testing.
- A ten-clip 1280 x 720 near-IR development slice from the second subject is now
  available privately. All Windows and Orin copies match, contain audio, and
  decoded fully. Coverage includes neutral/natural behavior, blink/closure,
  partial opening, clear glasses, closure plus gaze under glasses, head pose,
  distance/partial face and yawn. Initial 300-frame Windows runs had 100% face
  and semantic-landmark availability for YuNet/LBF and MediaPipe. The nominal
  blink and clear-glasses recordings include closures long enough to exercise
  prolonged-closure logic, so they are stress slices rather than clean normal-
  blink or neutral-eyewear ground truth until annotated. Private media and traces
  remain external to Git.
- Added a provider-neutral observation-quality gate. Quality loss is immediate;
  startup and reacquisition require a sustained usable interval and are exposed
  as `Recovering`. Recorded evaluation now passes landmark observations through
  this gate before eye temporal logic. Current providers still lack an
  independent eye-occlusion score, so this is the safe consumption mechanism,
  not a claim that landmark availability detects occlusion.
- Added configurable monotonic-time FSMs for yawn, hysteretic head-pose zones and
  directional counts, gaze and sustained distraction, driver presence, and
  combined drowsiness from prolonged closure/PERCLOS/recent yawns. All consume
  semantic inputs and explicit quality; none contains provider topology indices.
  Unknown or occluded inputs cannot create events. Head counts require a
  confirmed neutral return, and alert recovery is separately debounced.
- Deterministic sequence tests cover each new FSM, including single counting,
  duration gates, hysteresis/neutral rearming, distraction and recovery,
  presence confirmation, combined evidence, and unknown-quality suppression.
  Release validation at implementation commit `6d4f2c9` passed all 17
  configured CTests on Windows x64 and Orin aarch64, including real MediaPipe
  integrations, and all 16 applicable tests on native Ubuntu 24.04 x64. The
  Orin temporal-event test was confirmed as an ARM aarch64 executable.
  Recorded-data accuracy integration awaits semantic mouth/head/gaze adapters
  and the independent eye-quality observer.
- Added provider-specific topology adapters that emit semantic mouth/pose points
  and MediaPipe iris gaze, plus provider-neutral mouth openness, six-point head
  pose with reprojection quality and neutral calibration, and an eye-ROI image
  quality assessor. Providers without iris geometry report gaze unavailable.
- Recorded benchmark schema 4 now carries quality, mouth/yawn, calibrated pose
  and directional counts, gaze/distraction, presence, and combined drowsiness in
  private traces and anonymous aggregate JSON. Preliminary visible and IR
  development observations are recorded in `docs/STAGE20_SEMANTIC_RESULTS.md`.
- Pose-aware eye quality eliminated apparent prolonged closure during the tested
  IR head-motion slice while retaining one left/right/up/down count. The neutral
  visible slice produced no pose counts or distraction after calibration.
- Reliable in-frame hand/object eye occlusion remains an open gate requiring an
  annotated eye-ROI model benchmark. Raw recordings and traces remain external
  to Git, and no production-accuracy claim is made without event annotations.
- Semantic integration commit `277ed8f` passed all 21 configured Release CTests
  on Windows x64 and Orin aarch64 and all 20 applicable tests on native Ubuntu
  24.04 x64. The Orin head-pose estimator test was confirmed as an ARM aarch64
  executable.

## Sponsor recorded-video demonstration gate — 2026-08-31

### Objective

Provide a reproducible engineering demonstration on Windows x64 and Orin
aarch64 that accepts a sponsor-selected local video, displays the approved
Stage 20 provider-neutral states/events/counters while the recording plays,
and presents end-of-video statistics. The demonstration does not replace the
Stage 19/20 accuracy gates and must not claim production or safety readiness.

### Scope and privacy boundary

- Reuse the recorded-input Stage 20 pipeline and named approved policy; do not
  create a second set of event algorithms for the UI.
- Support YuNet/LBF and MediaPipe through their existing runtime contracts.
- Show unavailable provider capabilities explicitly rather than synthesizing
  an event from unsupported landmarks.
- Accept videos by local path. Recordings, audio, frames, traces, annotations,
  and per-video results remain outside Git.
- The user subsequently requested a single-folder/ZIP meeting package that
  includes videos for an offline Windows sponsor computer. Therefore, a curated
  recording subset may be included only in separately generated external
  meeting archives. Those archives remain outside Git, must identify their
  privacy status, and require permission to show the selected recording.
- Include launchers, an anonymous video catalog, package verification, and
  operator documentation.

### Acceptance gate

- A single documented command launches a local video on each platform.
- Playback shows driver presence, eye state/openness, blink/long-blink/
  prolonged-closure counts, PERCLOS when available, yawn, head zone/counts,
  gaze/distraction availability, monitoring availability, drowsiness, source
  time, backend and processing rate.
- End-of-video output records the same aggregate schema used by headless
  evaluation and the final display remains reviewable until the operator exits.
- `Q` or `Esc` exits cleanly; invalid paths and unavailable backends fail with
  actionable diagnostics.
- Focused and full Release tests pass on Windows and Orin. Both packages are
  checked for architecture, dependencies, models, and no direct MediaPipe
  bridge linkage.
- At least one private representative recording and one dashcam recording run
  successfully on each available platform. Private data and results are not
  committed.
- The external meeting archive contains all application/runtime payloads and
  its selected videos in one directory tree, has a payload checksum verifier,
  and passes verification plus self-test after a fresh extraction.
- User action is required only for final GUI observation/rehearsal on Windows
  and the Orin desktop after headless/build evidence passes.

### Implementation evidence (2026-09-01)

- Added a recorded-video sponsor display backed by the Stage 20 semantic/FSM
  pipeline, end-of-file handling, persistent summary, aggregate schema-5 output,
  and Windows/Orin launchers with a 15-video anonymous menu.
- Added external package support for curated video payloads, all-payload
  checksums, offline verification, Windows application-local runtime DLLs, and
  an Orin software H.264 fallback for recordings rejected by the automatic
  Jetson decoder path.
- Windows and Orin Release builds each passed all 23 CTests. Fresh external
  packages passed their manifests and deterministic self-tests; direct
  recorded-video runs succeeded with zero unrequested eye crops.
- The Windows ZIP and Orin archive, including private videos, remain outside
  Git. Checksums and detailed evidence are recorded in
  `docs/SPONSOR_DEMO_VALIDATION.md`.
- The implementation/package gate is complete. Final visual rehearsal on the
  different Windows sponsor computer and the Orin desktop remains the marked
  user-action gate.

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

### Driver-identification authorization — 2026-09-03

The product owner separated driver identification from later object/context
work and authorized Stage 21 Steps 21.0--21.3 on
`feature/stage21-driver-identification`. Scope includes documentation,
provider-neutral interfaces/tests, private local evaluation, and pretrained
baseline benchmarking. Training, threshold approval, merge, and release remain
excluded. The approved baseline is offline identification only, at most 50
drivers, portable profiles, photo/video/live enrollment, retained enrollment
images, generic DMS behavior for unknown drivers, controlled automatic profile
improvement, mandatory spoof protection, and initial Windows/Ubuntu/Orin
support for India. See `docs/STAGE21_DRIVER_IDENTIFICATION_PLAN.md` and
`docs/STAGE21_PRODUCT_PRIVACY_DECISION_RECORD.md`.

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
4. **Completed 2026-08-28 — Stage 20:** operational event definitions,
   thresholds, calibration behavior, HMI policy, and recognition separation
   approved as proposed.
5. **Before Stage 21 product integration:** approve recognition/privacy policy and object taxonomy.
6. **When Raspberry Pi is available:** confirm hardware, access and optional accelerator scope.

## Current recommendation

Do not select YuNet + LBF or the evaluated YuNet + PFLD candidate as the final production stack. Retain both as benchmarks. Use MediaPipe as the current eye-geometry reference while Stage 20 adds provider-neutral calibration, quality handling and temporal FSMs; run it at controlled cadence on Orin. A dedicated eye ROI model remains conditional on failure of the calibrated eye-opening, blink and PERCLOS gates.

The model-neutral metric and FSM mechanisms are implemented. The 34-clip
trace-level error triage is complete and recorded in
`docs/STAGE20_TRACE_ERROR_ANALYSIS.md`; private intervals and mappings remain
outside Git. Stage 20 is proceeding through annotation/scoring tooling and
trained eye-ROI occlusion option evaluation before the operational-definition
review.

Task 4 now has a privacy-preserving annotation contract, synthetic fixtures,
deterministic event/delay scoring, duration-weighted state scoring, and anonymous
slice aggregation in `docs/STAGE20_ANNOTATION_PROTOCOL.md`. Task 5
architecture evaluation recommends benchmarking a MobileNetV3-Small-class
multi-class eye-ROI model against a multi-task openness variant; details and the
training gate are in `docs/STAGE20_EYE_OCCLUSION_MODEL_OPTIONS.md`.

Task 4's synthetic workflow now also validates annotation files and computes
two-annotator event/boundary agreement with an explicit adjudication flag. The
remaining accuracy work depends on producing real timestamped annotations; the
tooling itself is complete for the current development-data stage.

Task 6 operational defaults and the display/count/record/alert matrix are
proposed for product-owner review in
`docs/STAGE20_OPERATIONAL_POLICY_RECOMMENDATION.md`. They remain engineering
defaults rather than compliance or production-accuracy claims until approved
and evaluated against timestamped annotations.

The product owner approved the Task 6 proposal without changes on 2026-08-28.
The blink/long-blink/prolonged-closure boundaries, PERCLOS and yawn policy,
pose/distraction/presence/availability thresholds, automatic calibration and
reacquisition, HMI matrix, recognition separation, and privacy boundary are now
the accepted Stage 20 operational policy. Production-accuracy claims still
require timestamped annotations and subject/session-disjoint evaluation.

### Approved-policy implementation validation — 2026-08-28

The approved policy is implemented as the named C++ profile
`stage20-approved-2026-08-28`. Ordinary blinks, long blinks and prolonged
closures have separate temporal behavior; blink refractory handling,
quality-gated eye calibration, calibration reset/reacquisition,
monitoring-unavailable timing, drowsiness hold/recovery, and schema-5 output
are covered by deterministic tests. The timestamping workflow is recorded in
`docs/STAGE20_TIMESTAMPING_OPERATOR_GUIDE.md`.

All 34 private development clips completed the schema-5 MediaPipe rerun with
zero execution failures. Recordings, subject mappings, clip-level results and
traces remain outside Git. The following anonymous aggregates may be retained:

| Slice | Schema | Frames | Detected | Unknown eye | Blinks | Long blinks | Prolonged events | Yawns | Distracted frames | Unavailable / notify frames | Mean FPS |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| subject A, visible | 4 | 11,100 | 99.20% | 5.97% | 58 | — | — | 2 | 402 | — | 34.99 |
| subject A, visible | 5 | 11,100 | 99.20% | 34.93% | 37 | 11 | 3 | 2 | 180 | 401 / 29 | 42.53 |
| subject B, IR | 4 | 7,200 | 96.22% | 18.25% | 52 | — | — | 4 | 888 | — | 46.30 |
| subject B, IR | 5 | 7,200 | 96.22% | 69.53% | 20 | 0 | 1 | 4 | 480 | 536 / 325 | 52.58 |
| subject B, visible | 4 | 9,900 | 96.48% | 16.44% | 82 | — | — | 4 | 1,143 | — | 62.41 |
| subject B, visible | 5 | 9,900 | 96.48% | 30.95% | 44 | 7 | 3 | 4 | 1,109 | 353 / 12 | 56.66 |

Schema 5 deliberately accepts fewer ordinary blinks by separating long and
prolonged closures and reduces distraction frames in two slices. Its stricter
calibration/quality handling also materially increases unknown-eye coverage,
most strongly on IR. This is a recorded validation risk, not evidence for
loosening thresholds without timestamped ground truth. The next accuracy gate
therefore remains the operator-timestamped, subject/session-disjoint scoring
workflow; these unannotated counts are diagnostic comparisons only.

Cross-platform build evidence at commit `3b61201`:

- Windows x64 Release: all 22 registered tests passed.
- NVIDIA Jetson Orin/Linux aarch64 Release with the packaged MediaPipe runtime
  and OpenCV 4.8.0: build passed and all 18 registered tests passed.
- Ubuntu 24.04 x64 Release with OpenCV 4.6.0 and the provider-neutral/default-
  off runtime path: build passed and all 18 registered tests passed. An
  Ubuntu-x64 MediaPipe runtime package was not available locally, so its
  enabled packaging path was not claimed as validated.

No training crops were extracted and no eye-ROI model was trained. Both remain
behind the separately required explicit product-owner approval.

### Eye-ROI crop readiness audit — 2026-09-01

The product owner subsequently approved local, non-Git eye-region crop
extraction for a benchmark, explicitly excluding model training. The extractor
produced 1,414 left/right pairs (2,828 crops) from six timestamped clips with
zero missing files, decode failures, or dimension errors. Anonymous audit and a
private balanced visual review found that event-interval-derived candidate
classes are not dense frame-level truth, the set is 75.42% unlabelled-visible,
only two subjects are represented, and the sole IR slice covers one
subject/session. Required partial, glasses, invalid-quality, domain-balanced
occlusion, and hard-negative crop labels are absent or insufficient.

The data is accepted for extractor QA and annotation-queue development but is
rejected as a training-ready or production-selection dataset. Random crop
splits are prohibited; both eye sides, neighboring frames, events, sessions,
and subjects must remain grouped. No training approval is requested. Detailed
anonymous conclusions and the next data gate are recorded in
`docs/STAGE20_EYE_ROI_CROP_AUDIT.md`; all images, mappings, per-crop data, and
contact sheets remain outside Git.

The six-clip schema-5 event checkpoint is summarized in
`docs/STAGE20_ACCURACY_CHECKPOINT.md`. Ordinary blink F1 is 0.806, long-blink F1
is 0.667, prolonged-closure F1 is 0.333, yawn F1 is 0.889, and explicit
eye-occlusion recall is zero. Horizontal gaze is partially supported; vertical
gaze and head-direction event matching are currently zero. Dense eye-state and
visibility truth is absent, so PERCLOS/openness and duration-weighted state
accuracy cannot yet be scored. These results keep the production-accuracy gate
open and do not justify changing the approved policy thresholds.

The pose/gaze semantic correction was subsequently validated at commit
`536f1a4` on Windows x64, Ubuntu 24.04 x64, and Orin aarch64. A deterministic
trace-to-prediction converter was added and reproduced the retained 126-row
pre-correction prediction CSV exactly. The full 8,501-frame rerun removed all
ten unmatched vertical-gaze predictions and created the expected physical
left/right/up head matches, but vertical-gaze recall remains zero, gaze-left
false positives increased, and eye-occlusion recall remains zero. Detailed
anonymous metrics and the decision to retain the approved thresholds are in
`docs/STAGE20_ACCURACY_GATE_RERUN.md`.

Dense eye-state/visibility annotation preparation now has a deterministic
session-grouped batch generator. The private 2,828-crop manifest was frozen by
checksum and expanded into independent templates for two anonymous annotators.
Each annotator receives three complete subject/session batches: 1,722 rows for
the first visible session, 290 rows for the IR session, and 816 rows for the
second visible session. Labels and templates remain outside Git. Human review,
independent second-pass completion, validation, and adjudication were completed
on 2026-09-03. Deterministic dense-state scoring now reports 87.82%
duration-weighted model-known coverage, 98.50% conditional state accuracy, and
86.51% end-to-end accuracy over 1,216 evaluable paired samples. Availability,
especially in the single IR session, is the dominant remaining eye-state
failure. Thresholds remain unchanged; broader fixed-window PERCLOS validation
and the conditional eye-ROI model gate remain open. The scorer produced
230 comparable 60-second windows on the only sufficiently long clip, with
0.0064 mean absolute PERCLOS error; the mostly-open single-session slice is
tooling evidence rather than a production gate. See
`docs/STAGE20_DENSE_EYE_STATE_RESULTS.md`.

The product owner approved preparation, local extraction, and double review of
a second private recording batch on 2026-09-03, still excluding model training.
The batch initializer reserves anonymous clips C07-C18 for three additional
subjects across visible eye-state, genuine IR, clear-glasses, and occlusion
sessions. The recording guide, consent-explicit inventory, checksum freeze
tool, and deterministic tests are committed. Recording and ingestion are now
the external data dependency; see `docs/STAGE20_SECOND_BATCH_RECORDING_GUIDE.md`
and `docs/STAGE20_STATUS.md`.

## Inputs currently unavailable

The requested `orin.txt` and `windows.txt` console logs were not present in the repository or supplied workspace when this plan was created. Stage 17 can begin from source instrumentation, but the logs should be added as non-secret validation inputs when available so their original runs can be correlated with the new measurements.
