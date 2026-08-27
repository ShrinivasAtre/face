# DMS Collaborator Testing Handoff

## Audience and purpose

This handoff is for Sanjana, who is independently exploring DMS tests and
gesture-related behavior with Shrinivas. It provides enough context to reproduce
tests and report useful evidence without changing the accepted architecture or
the frozen development plan.

Shrinivas and the primary development agent remain responsible for product
decisions, roadmap acceptance, production model selection, and integration.
Independent experiments are welcome, but they are evidence rather than accepted
product behavior until reviewed through the main project workflow.

## Repository checkpoint

- Repository: `https://github.com/ShrinivasAtre/face`
- Active follow-on branch: `feature/dms-roadmap-performance`
- Current recorded-input implementation commit: `072e597`
- Canonical completed baseline: `main`
- Build system: CMake for every native application and test
- Language/runtime requirement: production inference and DMS logic remain C++
- Supported architecture: Windows x64, Ubuntu x64, NVIDIA Jetson Orin aarch64,
  with Raspberry Pi planned behind the same interfaces

Before doing work, read these files completely:

1. `AGENTS.md`
2. `docs/DEVELOPMENT_PLAN.md`
3. `docs/NEXT_DEVELOPMENT_HANDOFF.md`
4. `docs/FOLLOW_ON_DEVELOPMENT_PLAN.md`
5. `docs/STAGE19_RECORDING_GUIDE.md`
6. `docs/STAGE19_DATASET_PROTOCOL.md`

Do not reopen or redesign the completed 16-step MediaPipe integration unless a
reproducible regression demonstrates a defect.

## Current technical direction

- YuNet plus LBF and the evaluated YuNet plus PFLD model remain benchmark
  baselines, not the selected production geometry stack.
- MediaPipe is the current eye-geometry reference. On Orin it must run at a
  controlled cadence because full-frame inference remains CPU-bound.
- DMS algorithms consume provider-neutral semantic observations. Do not put
  provider landmark indices, provider-specific EAR thresholds, or special-case
  model logic inside blink, PERCLOS, drowsiness, gaze, or gesture FSMs.
- Eye processing may run periodically, on uncertainty, after reacquisition, and
  around suspected transitions; it is not required on every camera frame.
- Unknown, occluded, low-confidence, stale, and missing observations must not be
  silently interpreted as open or closed eyes.

Stage 20 currently contains provider-neutral eye calibration, openness
hysteresis, duration-based blink confirmation, prolonged closure, rolling
PERCLOS, quality-state handling, and deterministic tests. The headless benchmark
now emits these values on a monotonic recorded timeline. Yawn, head pose, gaze,
distraction, presence, occlusion quality estimation, higher-level temporal
filtering and the combined drowsiness FSM remain in progress.

## Privacy and recording rules

- Never commit personal videos, audio, photographs, extracted frames, identity
  annotations, or per-frame traces.
- Keep recordings and derived artifacts outside the repository.
- Do not upload recordings to public services or third-party datasets without
  the subject's explicit authorization.
- Anonymous aggregate measurements and authorized source-file checksums may be
  proposed for the repository after review.
- Remove temporary extracted frames after the test unless retention was
  explicitly approved.
- A recording filename should use an anonymous subject/session identifier in
  production data. Personal names are acceptable only in private local folders.

## Recommended independent testing workflow

1. Create a branch from `feature/dms-roadmap-performance`; do not commit directly
   to `main` or rewrite shared branch history.
2. Keep a local manifest containing clip path, checksum, resolution, frame rate,
   duration, lighting, eyewear, camera placement, intended action and consent.
3. Run technical sanity checks before model testing: full decode, audio presence,
   duration, resolution, timestamp behavior and a full-timeline visual review.
4. Use `face_benchmark` without a GUI for repeatable provider comparisons.
5. Keep private JSON/CSV results outside Git. Summarize only aggregate evidence.
6. Repeat an experiment at least twice before reporting a performance or event
   count; record the exact commit, build type, command, target and model checksum.
7. Separate calibration recordings from evaluation recordings. Do not choose a
   threshold on the same frames used to report final accuracy.
8. Report failures and negative results. A provider returning landmarks during
   an occlusion is not proof that the eyes were observable.

Example benchmark command from the executable directory:

```text
face_benchmark --backend=mediapipe --input=<private-video> --warmup=30 --frames=300 --output=<private-result.json> --trace=<private-trace.csv>
```

Supported benchmark backends are `yunet`, `pfld` (with an explicit checksum-
pinned `--pfld-model`), and `mediapipe` when the build enables its packaged
runtime. Do not compare Debug results with Release results.

## Gesture experiments

Treat a gesture as a temporal state machine, not a single landmark or frame.
Each proposed gesture should define:

- purpose and safety relevance;
- observable inputs independent of a particular model topology;
- start, confirmation, hold, release and cancellation conditions;
- minimum duration, debounce, hysteresis and refractory period;
- behavior when the face, hands, eyes, or mouth are missing or occluded;
- allowed head motion, gaze motion and ordinary-driver confounders;
- count/display/record/alert behavior;
- expected false positives and false negatives;
- deterministic synthetic sequence tests;
- clip-level ground-truth annotation rules and evaluation metrics.

For hand or object gestures, keep face geometry, hand geometry and object
classification as separate observations. Associate them spatially and
temporally in a provider-neutral layer. Do not label an arbitrary object from
facial landmarks, and do not treat a hand near the mouth as a cigarette, drink,
or yawn without an explicit object/context observation.

Useful early gesture robustness checks include:

- gesture versus ordinary head movement;
- gesture versus natural blink/yawn/talking behavior;
- left/right hand and partial-hand visibility;
- near/far camera distance;
- glasses, glare and low light;
- face or hand leaving the frame;
- held gesture versus repeated gesture;
- simultaneous occlusion and gesture;
- variable frame rate, dropped observations and delayed inference.

## Reporting template

Provide the following for every result:

```text
Objective:
Commit and branch:
Target / OS / architecture:
Release or Debug:
Backend and model checksum:
Private clip ID and checksum:
Clip duration / resolution / nominal FPS:
Lighting / eyewear / camera placement:
Exact command or test procedure:
Expected events and annotated times:
Observed events and times:
Precision / recall / F1 or count error:
False alarms and likely confounders:
Mean / p95 latency, throughput and CPU use:
Reproduction count:
Private artifacts retained and location:
Conclusion and recommended next experiment:
```

Screenshots are supporting evidence only. Prefer machine-readable traces and
timestamped annotations for event evaluation.

## Changes that require coordination

Coordinate with Shrinivas before:

- changing the frozen roadmap or acceptance criteria;
- selecting or rejecting the production provider/model;
- changing event definitions or product thresholds;
- introducing a new runtime dependency, license, accelerator or training data;
- changing the C ABI, backend-neutral interfaces, packaging boundary or CMake
  architecture;
- committing aggregate results derived from personal recordings;
- merging into `main` or publishing binaries/data.

## Bootstrap prompt for an assistant

```text
Help me test the DMS project at https://github.com/ShrinivasAtre/face as an
independent collaborator. Work from feature/dms-roadmap-performance. Read
AGENTS.md, docs/DEVELOPMENT_PLAN.md, docs/NEXT_DEVELOPMENT_HANDOFF.md,
docs/FOLLOW_ON_DEVELOPMENT_PLAN.md, and docs/COLLABORATOR_TESTING_HANDOFF.md
completely before acting. Preserve the CMake/C++ and provider-neutral
architecture. Keep all personal recordings, audio, extracted frames,
annotations and per-frame traces outside Git. I am mainly investigating tests
and gesture behavior; treat my results as experimental evidence for review, not
as authorization to change the frozen product plan or merge to main.
```
