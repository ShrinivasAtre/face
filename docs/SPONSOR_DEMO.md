# Face DMS Sponsor Demonstration

## What this package demonstrates

This is a development demonstration, not a production or safety-certified Driver Monitoring System. It contains two complementary demonstrations:

1. A live-camera preview of the two current face/landmark backends: YuNet + LBF and MediaPipe Face Landmarker.
2. A deterministic, privacy-safe test of the Stage 20 provider-neutral temporal eye engine using synthetic semantic EAR samples.

No personal photograph, recording, extracted frame, biometric annotation, or audio is included. The synthetic CSV is generated test data and does not describe a person.

## Quick start — Windows x64

Open PowerShell in the extracted package directory.

```powershell
.\run_self_test.ps1
.\run_face.ps1 yunet
.\run_face.ps1 mediapipe
```

Connect one camera before starting a live run. Press `Q` or `Esc` in the video window to exit. Run only one backend at a time.

Prerequisites: Windows x64 and a Microsoft Visual C++ runtime compatible with the packaged executable. OpenCV and model assets are included.

## Quick start — NVIDIA Jetson Orin

Open a desktop terminal in the extracted package directory.

```bash
chmod +x run_self_test.sh run_face.sh yunet_demo dms_sponsor_selftest
./run_self_test.sh
./run_face.sh yunet
./run_face.sh mediapipe
```

Connect one V4L2 camera before starting a live run. Press `Q` or `Esc` in the video window to exit. The GUI requires an active desktop display session.

Prerequisites: a JetPack-compatible Linux aarch64 system with the OpenCV 4.8, EGL/GLES, and JetPack multimedia shared libraries used by the build.

## Expected deterministic result

`run_self_test` must finish with `SELF-TEST PASSED`. It demonstrates:

- calibrated EAR-to-openness normalization;
- open/closed hysteresis;
- duration-based blink confirmation and single counting;
- explicit unknown state for occlusion, which cannot create a blink;
- prolonged-closure timing; and
- rolling PERCLOS calculation over known-quality intervals.

The package manifest contains SHA-256 checksums for every payload file.

## Current project status

- Stages 1–16: original CMake application and optional runtime-loaded MediaPipe bridge completed on Windows x64 and Orin aarch64.
- Stage 17: performance characterization completed. MediaPipe VIDEO mode improved throughput, but full-frame MediaPipe remains CPU-bound on Orin at about 5–6 FPS. YuNet/LBF is the real-time baseline, not the selected final landmark stack.
- Stage 18: provider-neutral observations, timestamps, quality states, independent task cadence, and bounded latest-frame scheduling completed on Windows, Ubuntu x64, and Orin.
- Stage 19: PFLD comparison is preliminary. The evaluated PFLD candidate did not pass the replacement gate. Broader production-representative annotated data is still required.
- Stage 20: eye calibration, openness, temporal blink FSM, prolonged closure, rolling PERCLOS, recorded-input timing, and deterministic tests are implemented. Dataset-level acceptance remains in progress.

Private development recordings have been used locally for engineering checks, including visible-light, low-light, glasses, head/gaze, and near-IR slices from more than one subject. They are intentionally absent from this package and from Git.

## Important live-preview limitation

The current camera window uses the legacy geometric EAR diagnostic counter. It is useful for comparing face tracking, responsiveness, and backend performance, but its blink count is known to produce false positives/misses and must not be presented as production accuracy. The deterministic self-test exercises the newer Stage 20 temporal engine. Connecting that engine to a calibrated, quality-gated production live pipeline is ongoing work.

## Future path

1. Complete Stage 20 operational definitions and connect provider-neutral eye quality/calibration to the live and recorded pipeline.
2. Annotate a larger visible-light/IR multi-person dataset; report event precision, recall, F1, false alarms per hour, count error, delay, and robustness slices.
3. Add yawn, head pose/movement, gaze, distraction, presence, and occlusion temporal FSMs.
4. Select the production facial/eye model from measured target-device evidence; consider a scheduled eye-ROI model rather than running it every frame.
5. Add consented recognition and separate cigarette/hand-held-object models after privacy policy and object taxonomy approval.
6. Produce final native packages for Windows, Ubuntu x64, Orin, and Raspberry Pi 5; Raspberry Pi claims wait for device validation.
7. Run sustained performance, thermal, memory, and final camera acceptance gates on every target.

## Evidence and source

The authoritative accepted plan and detailed validation evidence are in `docs/FOLLOW_ON_DEVELOPMENT_PLAN.md` in the source repository. The package revision and platform are recorded in its manifest. This demonstration should always be identified as an engineering checkpoint from the feature branch, not as a released DMS product.
