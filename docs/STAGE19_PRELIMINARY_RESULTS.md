# Stage 19 Preliminary Target-Clip Results

## Scope and privacy

This is a development-data gate, not a production model selection. The local
set contains one consented subject, visible light, one room/camera position,
clear-glasses and narrated action clips. The twelve raw recordings, audio,
frames, traces and annotations remain external to Git. Only anonymous aggregate
results and authorized recording checksums are recorded here.

The recordings provide 1920 x 864 H.264 video at approximately 30 FPS and AAC
audio. Their combined coverage includes neutral behavior, deliberate blinks,
short and prolonged closure, partial closure, gaze, head pose, mouth/yawn,
occlusion, cup/object interaction, distance/partial face, clear glasses and
natural combined behavior. Near-IR, darkness/transition, sunglasses,
vehicle-mounted capture and subject diversity remain missing production slices.

## Frozen providers and implementation

- baseline: YuNet face detection plus LBF landmarks;
- candidate: the checksum-pinned YuNet plus 68-point PFLD ONNX provider;
- reference: the accepted MediaPipe Face Landmarker runtime.

All three use the same `FaceBackend` result, semantic-eye contract,
`BlinkTracker`, input frames and diagnostic EAR threshold of 0.27. The benchmark
can emit a private frame trace containing timestamps, availability, EAR, eye
state, diagnostic count, face box and latency. PFLD model preprocessing and
iBUG topology remain inside its provider/mapper boundary.

## Performance results

Release builds used 300 measured frames after 30 warm-up frames. Backend latency
excludes capture/decode and semantic processing. Orin used a lossless temporary
PNG sequence because its default H.264 GStreamer route rejected these otherwise
valid MP4 files; the benchmark now supports deterministic image sequences.

| Target / sequence | Provider | Detection | Backend mean ms | p95 ms | Throughput FPS |
| --- | --- | ---: | ---: | ---: | ---: |
| Windows x64 / natural | LBF | 300/300 | 162.71 | 184.62 | 5.57 |
| Windows x64 / natural | PFLD | 300/300 | 183.53 | 286.12 | 5.00 |
| Windows x64 / natural | MediaPipe | 300/300 | 14.12 | 18.04 | 32.89 |
| Orin aarch64 / natural | LBF | 300/300 | 106.67 | 109.90 | 6.39 |
| Orin aarch64 / natural | PFLD | 300/300 | 102.48 | 105.03 | 6.56 |
| Orin aarch64 / natural | MediaPipe | 300/300 | 165.80 | 193.44 | 4.56 |

Throughput includes input acquisition and semantic processing, so the Orin PNG
decode cost (about 48--51 ms/frame) must not be attributed to a provider.
MediaPipe is decisively fastest on Windows but remains the slowest Orin path,
reproducing the live-camera performance problem. PFLD's small Orin advantage
over LBF is not large enough to decide the geometry stack on speed alone.

A complete 1,344-frame Windows blink pass exposed sustained behavior:

| Provider | Detection / semantic validity | Backend mean ms | p95 ms | FPS |
| --- | ---: | ---: | ---: | ---: |
| LBF | 1,344/1,344 | 256.29 | 379.49 | 3.71 |
| PFLD | 1,344/1,344 | 195.92 | 350.72 | 3.90 |
| MediaPipe | 1,344/1,344 | 13.67 | 15.43 | 41.62 |

The large LBF/PFLD sustained p95 values reinforce Stage 18's bounded-cadence
architecture: full-resolution face detection plus landmarks must not be forced
onto every captured frame in production.

## Eye-signal and diagnostic event results

The narrated blink clip contains ten deliberate blinks. The current diagnostic
logic increments on every open-to-closed threshold transition and has no
minimum-duration, reopening, refractory, confidence or occlusion FSM. With the
same 0.27 threshold:

| Provider | Valid frames | Mean EAR | EAR standard deviation | Frames below threshold | Diagnostic count |
| --- | ---: | ---: | ---: | ---: | ---: |
| LBF | 1,344 | 0.30 | 0.05 | 413 | 172 |
| PFLD | 1,344 | 0.21 | 0.05 | 1,279 | 30 |
| MediaPipe | 1,344 | 0.22 | 0.06 | 1,161 | 12 |

PFLD materially reduces LBF false transitions but does not solve the eye-event
problem. Its open-eye EAR scale is mostly below the frozen threshold, showing
that this candidate's six iBUG eye points do not provide a drop-in calibrated
eye-openness signal. MediaPipe is closest to the narrated count but still fails
a production acceptance gate.

Equal 300-frame clear-glasses and gaze/head-motion samples had 100% semantic
availability for every provider. Diagnostic counts remained unstable: glasses
LBF/PFLD/MediaPipe counts were 45/44/5, and gaze/head-motion counts were
40/29/8. Clear glasses therefore did not cause acquisition failure, but neither
LBF nor PFLD isolated eye closure from ordinary motion.

These counts are diagnostic evidence, not blink precision/recall. Frame-accurate
event matching, PERCLOS, yawn and head-pose errors require adjudicated timestamps
and the temporal algorithms intentionally scheduled for Stage 20. Reporting the
single-frame threshold as production accuracy would be misleading.

## Preliminary decision

Do not replace LBF with this PFLD candidate. It provides a modest Orin latency
improvement and better diagnostic blink count than LBF, but its eye-opening
scale and motion sensitivity fail the intended drop-in gate. Retain all three
providers as benchmark references while Stage 20 implements provider-neutral
temporal filtering/FSMs. Use MediaPipe as the current eye-geometry reference on
Windows; on Orin, run it at a controlled cadence and separately investigate
acceleration. If calibrated MediaPipe eye geometry still misses PERCLOS/blink
targets, evaluate a dedicated eye-ROI model rather than provider-specific DMS
exceptions.

This direction remains preliminary until the user reviews it. Final production
selection also requires multiple held-out subjects, intended visible/IR cameras,
lighting/eyewear/occlusion slices, frame-level annotations and product-approved
accuracy/latency weights.
