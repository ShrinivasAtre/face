# Face DMS Sponsor Demonstration

## What this package demonstrates

This is a development demonstration, not a production or safety-certified Driver Monitoring System. It contains three complementary demonstrations:

1. A live-camera preview of the two current face/landmark backends: YuNet + LBF and MediaPipe Face Landmarker.
2. A deterministic, privacy-safe test of the Stage 20 provider-neutral temporal eye engine using synthetic semantic EAR samples.
3. A sponsor-selected recorded-video demonstration using the approved Stage 20
   calibration, quality, metrics and event FSM pipeline, with live overlays and
   end-of-video JSON statistics.

The repository and source-built package contain no private media. The external
meeting ZIP deliberately adds the separately authorized demonstration videos;
those recordings may contain identity and audio. Extracted frames and biometric
annotations are not included. The synthetic CSV is generated test data.

## Quick start — Windows x64

Open PowerShell in the extracted package directory.

```powershell
.\verify_package.ps1
.\run_self_test.ps1
.\run_video_demo.ps1 mediapipe
.\run_face.ps1 yunet
.\run_face.ps1 mediapipe
```

`run_video_demo.ps1` displays a numbered menu of the videos included in the
external demonstration ZIP. It accepts an optional path as its second argument.
Press `Q` or `Esc` to stop. A timestamped aggregate result is written under
`results\`; no recording or frame is copied there.

Connect one camera before starting a live run. Press `Q` or `Esc` in the video window to exit. Run only one backend at a time.

Prerequisites: Windows 10/11 x64 with Media Foundation support for MP4. OpenCV,
the required Visual C++ runtime DLLs, application binaries and model assets are
included for offline use.

## Quick start — NVIDIA Jetson Orin

Open a desktop terminal in the extracted package directory.

```bash
chmod +x run_self_test.sh run_video_demo.sh verify_package.sh run_face.sh \
    yunet_demo face_benchmark dms_sponsor_selftest
./verify_package.sh
./run_self_test.sh
./run_video_demo.sh mediapipe
./run_face.sh yunet
./run_face.sh mediapipe
```

The Orin launcher provides the same numbered video menu and writes aggregate
results under `results/`. It requires an active desktop display session.

Connect one V4L2 camera before starting a live run. Press `Q` or `Esc` in the video window to exit. The GUI requires an active desktop display session.

Prerequisites: a JetPack-compatible Linux aarch64 system with the OpenCV 4.8,
EGL/GLES and JetPack multimedia shared libraries used by the build, plus the
GStreamer `qtdemux`, `h264parse`, `avdec_h264`, `videoconvert` and `appsink`
plugins. The recorded-video reader uses this software H.264 fallback when the
automatic Jetson MP4 path rejects a recording.

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

Private development recordings have been used locally for engineering checks,
including visible-light, low-light, glasses, head/gaze, and near-IR slices from
more than one subject. They remain absent from Git. A curated, renamed subset is
included only in the separately generated meeting ZIP.

## Recorded-video display

The display reports driver presence, calibrated eye state/openness, ordinary
blink, long-blink and prolonged-closure counts, PERCLOS when sufficient known
coverage exists, yawn count, head zone and directional counts, gaze and
distraction availability, monitoring availability, drowsiness state, source
time, backend and processing rate. Unsupported provider capabilities are shown
as unavailable rather than guessed.

On Windows, MediaPipe is the recommended demonstration backend because it
provides iris gaze and the strongest current eye geometry. On Orin, MediaPipe
full-frame CPU inference is expected to run at approximately 5--6 FPS; YuNet is
the responsive alternative but does not provide iris gaze and has weaker eye
accuracy. This is an engineering tradeoff, not a hidden demo setting.

The distributable ZIP may contain explicitly selected private demonstration
videos. Those videos and the ZIP remain outside Git. Confirm permission to show
the selected recording to the sponsor before playback.

## Important live-camera preview limitation

The separate camera window still uses the legacy geometric EAR diagnostic
counter. It is useful for backend responsiveness only and must not be presented
as production accuracy. The recorded-video demo uses the newer Stage 20
pipeline.

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
