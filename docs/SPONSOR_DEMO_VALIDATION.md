# Sponsor Recorded-Video Demo Validation Evidence

Validation date: 2026-09-01

Packaged source revision: `6eb95ab18e597dd1abfc689080bc2cfdb3545930`

## Privacy boundary

The repository contains no personal photographs, recordings, audio, extracted
frames, private annotations, or per-frame private traces. The external meeting
archives deliberately contain 15 separately curated recordings and therefore
remain outside Git. Their descriptive filenames indicate intended content, not
verified ground truth. Permission to show the sponsor-selected recording must
be confirmed before playback.

An eye-crop opt-in regression was found during validation: an unopened output
stream could be treated as enabled and write one crop without the explicit
option. The untracked crop was deleted immediately and was never committed or
pushed. The gate now checks `is_open()`. Subsequent Windows and Orin recorded
runs wrote zero eye crops without `--eye-crops-dir`.

## Common acceptance evidence

- The sponsor mode processes one recording to end of file, renders live DMS
  state/counters, and writes an aggregate schema-5 JSON result.
- The display includes presence, monitoring availability, eye state/openness,
  ordinary/long/prolonged closure events, PERCLOS, yawn, head zone and movement
  counts, gaze/distraction availability, drowsiness, processing rate, and the
  current event.
- `Q` and `Esc` stop playback cleanly; a completed recording holds its summary
  until a key is pressed.
- Each package has an offline payload manifest and verifier, a deterministic
  self-test, launchers, model assets, backend runtime, video catalog, and 15
  curated videos.
- The 15 videos occupy approximately 277 MB before packaging. No extracted
  frames, annotations, or per-frame traces are included.

## Windows x64

- Clean Release build root: `build-sponsor-demo`
- Full CTest: 23/23 passed
- Built-executable automated GUI run: 120/120 frames detected and rendered;
  yawn count 1; eye-crop pairs 0
- Packaged-executable automated GUI run: 120/120 frames detected and rendered;
  yawn count 1; eye-crop pairs 0
- Fresh archive extraction: 15 videos present, 35/35 payload hashes passed, and
  the extracted deterministic self-test exited successfully
- Package includes app-local OpenCV, bridge/model assets, and required Visual
  C++ runtime DLLs; the remaining prerequisites are Windows 10/11 x64 system
  DLLs and Media Foundation codecs
- External archive: `face-dms-sponsor-video-demo-windows-x64.zip`
- Archive size: approximately 350 MB
- Archive SHA-256:
  `c1c33243dd21b4d77b3e7edfc0afaacb16e477eb6b03017de0904b89df3609e3`

## NVIDIA Jetson Orin / Linux aarch64

- Clean Release build root: `~/common/p21-sponsor-video-demo/face/build-orin`
- Full CTest: 23/23 passed
- Binary architecture: Linux aarch64; dependency and direct-bridge-link checks
  passed during packaging
- The automatic Jetson MP4 path rejected the selected H.264 recordings. An
  explicit GStreamer software fallback (`qtdemux`, `h264parse`, `avdec_h264`,
  `videoconvert`, `appsink`) was added and verified.
- Direct MediaPipe MP4 run: 100/100 frames detected, yawn count 1, eye-crop
  pairs 0, 6.19 FPS
- Packaged direct MediaPipe MP4 run: 100/100 frames detected, yawn count 1,
  eye-crop pairs 0, 6.39 FPS
- Package verification: 30/30 payload hashes passed; deterministic self-test
  passed
- Earlier full-sequence checks processed 1,320/1,320 representative frames and
  118/118 dashcam frames. Temporary extracted private frame sequences and a
  failed temporary transcode were deleted after direct MP4 validation.
- External archive: `face-dms-sponsor-video-demo-orin-aarch64.tar.gz`
- Archive size: approximately 333 MB
- Archive SHA-256:
  `06ae0c4f723c212ea154c4a6ce573dec0ac61e35c1f73c35b8d7b6df9ee84eea`

## Remaining physical acceptance

One final visual rehearsal is required on the actual Windows sponsor computer
and at the Orin desktop. Verify window creation, readable overlay scaling,
smooth-enough playback for the selected backend, end-of-video summary, audio
expectations, and clean `Q`/`Esc` exit. This is the only remaining sponsor-demo
acceptance item requiring user action.

## Scope of the claim

This gate proves native builds, deterministic behavior, package completeness,
direct recorded-video operation, aggregate output, and the privacy boundary. It
does not prove production DMS accuracy. Event accuracy remains subject to the
Stage 20 timestamped, subject/session-disjoint acceptance gate. MediaPipe on
Orin remains CPU-bound at roughly 5--6 FPS, while YuNet is the responsive but
lower-eye-fidelity demonstration alternative.
