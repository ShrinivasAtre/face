# Stage 24 display, ROI and statistics validation

Date: 2026-09-04

## Scope and claim boundary

This report validates the committed Stage 24 configuration, processing-ROI and
eye/blink statistics checkpoint through commit `8ce798c`. It is engineering
evidence, not a production UI approval, safety claim, threshold change or
release authorization. The illustrative ROI profile is tied to the accepted
engineering image and is not a recommended camera/vehicle ROI.

## Deterministic contract coverage

- schema-versioned configuration loads the shipped example and rejects missing
  schema, duplicate/unknown keys, malformed values and invalid rectangles;
- disabled processing ROI preserves the full-frame matrix bytes/layout and
  leaves face/landmark coordinates unchanged;
- enabled ROI crops before inference, rejects insufficient face coverage and
  restores accepted boxes/landmarks to full-frame coordinates;
- cumulative and rolling eye-open percentages exclude Unknown and over-gap
  intervals, expose known-time coverage and reject non-monotonic updates;
- accepted blink timestamps are counted cumulatively and within the configured
  rolling window;
- transient face loss does not reset statistics; explicit reset starts a new
  confirmed-driver/session epoch;
- display configuration affects rendering only and does not disable detection,
  trace generation, monitoring FSMs or benchmark output.

## Windows x64 Release gate

- Toolchain: MSVC 19.51, Windows SDK 10.0.26100.0, OpenCV 4.8.0.
- Fresh build directory: `build-stage24-validation`.
- Result: 29 of 29 registered tests passed. Two PowerShell workflow tests are
  Windows-only.
- Validation image SHA-256:
  `525257a2263a0bfb1aecba45cfbfe5b0387aa16cb94216dc334e5c46dfa69e7c`.
- Full-frame and enabled-ROI smoke runs each detected 20 of 20 measured frames.
- Full-frame face box was `243,437,342,451`; restored ROI result was
  `244,434,342,457`. The small detector variance is expected because inference
  input differs; both results are expressed in the original 960x1280 frame.

Three repeated 30-frame YuNet/LBF resource runs used five warm-up frames and a
100 ms resource sample interval:

| Mode | Detections | Mean FPS | Mean backend ms | Mean process CPU, total capacity | Maximum sampled RSS MiB |
|---|---:|---:|---:|---:|---:|
| Full frame | 90/90 | 6.08 | 161.35 | 41.41% | 159.8 |
| Illustrative ROI | 90/90 | 23.72 | 38.92 | 33.88% | 92.3 |

## NVIDIA Orin aarch64 Release gate

- Exact checkout: detached commit `8ce798c` under
  `~/common/p24/face-stage24`.
- Compiler: GCC 13.3.0; OpenCV 4.8.0; accepted Stage 17 MediaPipe aarch64
  runtime package enabled.
- `face_benchmark` was confirmed as an ARM aarch64 ELF executable.
- Result: 27 of 27 applicable tests passed.
- The same validation-image checksum matched on-device.

Three repeated 20-frame YuNet/LBF resource runs used five warm-up frames and a
100 ms resource sample interval:

| Mode | Detections | Mean FPS | Mean backend ms | Mean process CPU, total capacity | Maximum sampled RSS MiB |
|---|---:|---:|---:|---:|---:|
| Full frame | 60/60 | 13.91 | 68.93 | 48.81% | 180.98 |
| Illustrative ROI | 60/60 | 41.65 | 21.06 | 33.67% | 109.12 |

These short repeated runs show that the ROI path is active and measurable on
both targets. They do not replace target-camera accuracy, representative
recorded-video validation or sustained thermal/resource benchmarking.

## Gate conclusion

Stage 24 is engineering-complete for the committed configuration, processing
ROI, display-selection and eye/blink statistics scope. Product defaults and UI
wording remain a product-owner decision. Confirmed-driver-change reset remains
ready at the statistics API but will be connected only when the identification
pipeline supplies a stable identity-change event.
