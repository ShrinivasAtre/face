# Stage 19 DMS Dataset and Benchmark Protocol

## Authorization boundary

No recording may be copied, transformed, annotated, retained, or committed
until the owner authorizes its use and states retention/privacy restrictions.
Images, videos, extracted frames, annotations containing identity, and derived
biometric landmarks remain external validation data by default.

The repository may contain only schemas, non-identifying aggregate results,
checksums, commands, model manifests, and code unless the user explicitly
authorizes otherwise.

## Minimum representative data

Use fixed recordings or frame sequences so every provider sees identical input.
Include visible-light and intended near-IR cameras at representative resolution,
distance, mounting angle, and frame rate. Cover multiple subjects and sessions
where policy permits, without using training subjects as the final selection
test set.

Required slice labels are:

- visible light, near-IR, darkness, and illumination transition;
- no glasses, clear glasses, and sunglasses;
- frontal, yaw left/right, pitch up/down, roll, and combined pose;
- near/nominal/far distance and full/partial/out-of-frame face;
- unoccluded, eye occlusion, mouth occlusion, hand/object occlusion;
- eyes open/closed/partially open, deliberate blinks, natural blinks, and
  prolonged closure;
- mouth closed/open, speech, and yawn;
- stillness and natural vehicle-like vibration/head motion.

## Annotation contract

Each recording has an anonymous subject/session identifier, camera modality,
capture metadata, monotonic frame timestamps, and slice labels. Frame-level
ground truth records face visibility, landmark visibility, eye opening or
open/closed state, mouth opening/yawn state, head pose, and occlusion. Event
annotations record blink, yawn, and directional head-movement start/end times.

Annotators must mark `unknown/not-visible` rather than infer hidden anatomy.
Double-annotation and adjudication are required for the final test subset.
Inter-annotator agreement is reported alongside model results.

## Frozen comparisons

- YuNet face detection plus LBF landmarks (baseline).
- YuNet face detection plus the checksum-pinned PFLD candidate.
- Accepted MediaPipe backend (reference candidate).

All providers feed the same backend-neutral semantic observations and downstream
metric/event implementations. Provider-specific thresholds, topology leakage,
or different event definitions invalidate the comparison.

## Required accuracy metrics

- normalized landmark error, failure rate, and temporal jitter;
- eye-opening absolute error and open/closed ROC and precision-recall curves;
- blink precision, recall, F1, false events per minute, missed events per minute,
  and absolute count error;
- PERCLOS absolute error over fixed, identically aligned windows;
- yawn precision, recall, F1, false events, missed events, and count error;
- head-pose yaw/pitch/roll angular error and directional movement-count error;
- each metric overall and for every required robustness slice.

Threshold selection uses a calibration subset. The held-out test subset is
evaluated once with frozen thresholds. Subject/session separation prevents
identity leakage.

## Required performance metrics

For each available target and provider, use a Release build and report input
resolution, processed/skipped frames, FPS, p50/p95/p99 inference and end-to-end
latency, CPU/GPU utilization, RSS/peak memory, power mode/draw where available,
temperature, throttling, and sustained-run duration. Scheduling cadence is
reported explicitly; a provider that skips frames is not compared as if it ran
on every frame.

## Selection rule

Before scoring, the user approves product weights and hard minimums for missed
closure/blink/yawn events, false alarms, PERCLOS error, latency, and platform
feasibility. PFLD replaces LBF only if it passes every hard minimum and has the
best weighted score without making a required target impractical. Otherwise LBF
is retained temporarily, MediaPipe remains a candidate, or Stage 20 evaluates a
dedicated eye model.

## User input required before dataset work

The user must identify the available recordings (or confirm none exist),
authorize their use, and specify:

- allowed machines and people;
- whether frames may be extracted or only streamed/read in place;
- retention deadline and deletion requirements;
- whether anonymous aggregate metrics and checksums may be committed;
- whether face identity/recognition annotations are excluded from Stage 19;
- whether new recording is permitted if coverage is insufficient.
