# Live-driver Windows and Orin resource-comparison runbook

Date: 2026-09-07

## Status and claim boundary

The commands are ready; execution waits for the user to provide a representative
driver and make the physical camera available on each target. These runs measure
engineering behavior. They do not establish safety limits or production accuracy.
Generated JSON/CSV/telemetry stays below the ignored `outputs/` directory or an
equivalent private location; only approved anonymous aggregates may be committed.

## Physical sequence

1. Use the intended camera, resolution and mounting position. Record the camera
   model, target power mode and anonymous session ID outside filenames containing names.
2. Seat the driver normally. Include stable forward/open-eye calibration, natural
   blinking, head movement and ordinary monitoring. Do not stage unsafe driving behavior.
3. Run Windows first with the full-frame baseline. If a reviewed vehicle ROI file
   is available, pass it as `-PresentationConfig` for paired full/configured runs.
4. Move the same camera to Orin without changing its mounting geometry and repeat.
5. The scripts run three 900-frame repetitions per mode and record frame traces,
   resource traces, JSON summaries and checksums. Orin also starts `tegrastats` when available.
6. Review face-detection coverage before interpreting calibration or event phases.
   A no-driver/poor-view run is capture evidence only.

## Windows command

```powershell
scripts\run_live_resource_comparison.ps1 `
  -Benchmark build-stage24-validation\Release\face_benchmark.exe `
  -Camera 0 `
  -OutputRoot outputs\live-driver\windows
```

Add `-PresentationConfig config\vehicle-reviewed.conf` only after its driver-seat
rectangle has been reviewed on the live view.

## Orin command

```bash
scripts/run_live_resource_comparison.sh \
  build-stage24/face_benchmark 0 outputs/live-driver/orin
```

Pass a fourth argument for a reviewed presentation/processing configuration.

## Required review output

- detection coverage and calibration completion;
- total and per-core CPU during calibration and processing;
- RSS range/growth and thread count;
- capture, provider, semantic/FSM and end-to-end latency distributions;
- frame drops/superseded frames;
- Orin temperature, power and throttle evidence;
- full-frame versus configured-ROI comparison, when a reviewed ROI exists.
