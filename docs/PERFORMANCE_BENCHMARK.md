# Stage 17 Performance Benchmark

## Purpose

`face_benchmark` runs the production backend and semantic eye/blink path without a camera or GUI. It accepts a still image (repeated without decode cost) or a video (decoded sequentially and restarted at end), performs warm-up frames, and writes versioned JSON results.

Schema version 6 also feeds the provider-neutral Stage 20 eye metric FSM. Video
presentation timestamps are converted to a strictly monotonic recorded timeline;
duplicate, missing, backward, and loop-reset timestamps advance by the nominal
frame period. Temporal durations therefore describe recording time rather than
inference wall-clock time. The private CSV trace adds eye usability, normalized
openness, open/closed/unknown state, confirmed blink events/count, closure
duration, prolonged closure, PERCLOS and known-time coverage.

Current backend results do not expose an eye-occlusion confidence. Until a
separate provider-neutral quality observation is connected, successfully mapped
landmarks are marked usable and missing landmarks are marked missing. Occlusion
results must not be interpreted as production accuracy from geometry availability
alone.

The accepted cross-platform validation image is external to Git:

- filename: `IMG-20150331-WA0001.jpg`
- dimensions: `960x1280`
- SHA-256: `525257a2263a0bfb1aecba45cfbfe5b0387aa16cb94216dc334e5c46dfa69e7c`

The input must be rejected as validation evidence if its checksum differs. Camera-resolution measurements remain separate because this image is larger than the established `640x480` camera stream.

## Build

Configure an enabled Release build using the platform's accepted MediaPipe runtime package. Build and run all CTests before benchmarking.

## Command

```text
face_benchmark --backend=yunet|mediapipe --input=<image-or-video> --warmup=10 --frames=100 --output=<result.json>
```

For periodic CPU/core, memory and thread sampling, add:

```text
--resource-profile --resource-trace=<private-resource-samples.csv> --resource-sample-ms=200
```

`--resource-profile` enables periodic sampling; specifying `--resource-trace`
also enables it. The resource trace contains elapsed time, operational phase, process CPU as a
percentage of total machine capacity, resident/private bytes, process thread
count, and a semicolon-separated utilization value for every logical core.
Keep raw traces outside Git when they can be associated with private input.

The executable locates the deployed models and runtime bridge relative to itself, exactly like `yunet_demo`.

## Result contract

Schema version 6 records:

- backend, optimized/debug build configuration, input kind and decoded dimensions;
- warm-up, measured, successful and detected frame counts;
- dropped, superseded and rendered frame counts (all zero for this synchronous headless harness);
- measured throughput;
- process CPU as a percentage of total logical-CPU capacity;
- logical CPU count, initial/final/peak resident memory and resident-memory growth;
- mean, p50, p95, p99, minimum and maximum latency for input acquisition, backend processing, semantic eye/blink processing and end-to-end processing.
- source revision, target platform, compiler and resource-sampling interval;
- sampled resource summaries overall and for startup, warm-up, calibration,
  processing and recalibration;
- component latency for face geometry, eye mapping/EAR, eye quality/calibration,
  temporal FSMs and output in addition to the stable aggregate timings.

`calibration` is the initial quality-gated eye-open calibration period.
`recalibration` begins after an accepted calibration is reset by the existing
absence/invalid-geometry policy. A phase with zero samples was shorter than the
configured sampling interval or was not entered; it must not be interpreted as
zero resource usage.

The backend measurement includes any bridge-side BGR-to-RGB conversion, inference and result conversion. Bridge-internal timings may be added as a compatible diagnostic extension, but must not replace the end-to-end measurement.

The accepted photograph is personal validation data and must remain outside Git. Its filename, dimensions and SHA-256 above are the reproducibility contract; evidence is invalid when any of those differ.

## Comparison rules

1. Use Release/optimized builds.
2. Use identical input bytes, warm-up and measured frame counts.
3. Record platform metadata, power mode and thermal state alongside results.
4. Run at least three repetitions when selecting a production candidate.
5. Compare p95/p99 and detection compatibility, not mean FPS alone.
6. Do not mix still-image throughput with live-camera FPS.
7. A candidate with missing detections or unsuccessful frames fails regardless of speed.
