# Stage 20 operational threshold and event-policy recommendation

Date: 2026-08-28
Status: approved by the product owner on 2026-08-28; not a production-accuracy claim

## Scope

These are conservative engineering defaults for the current provider-neutral
DMS. They define semantics and testable behavior; annotated evaluation may
change numerical thresholds. They do not establish regulatory compliance.

EU DDAW rules require minimizing errors under real driving conditions, day and
night operation, closed-loop data minimization, and a warning by the applicable
drowsiness level. They also state that normal DDAW operation must not require
biometric information such as facial recognition. UN Regulation No. 171 adds
driver-engagement monitoring for supported assistance systems. Consequently,
facial recognition must remain an optional, separately governed product feature
and must not be required by the DMS state/event pipeline.

References:

- [EU Delegated Regulation 2021/1341 (DDAW)](https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX%3A32021R1341)
- [UNECE summary of UN Regulation No. 171](https://unece.org/media/transport/Vehicle-Regulations/press/395206)

## Global observation policy

1. `Unknown`, `missing`, `low-confidence`, and `occluded` are not open, closed,
   forward, or away. They must not create or complete a blink, yawn, head, gaze,
   distraction, or drowsiness event.
2. PERCLOS excludes unknown time from the numerator and denominator and is
   publishable only when known coverage meets its gate.
3. Cached observations expire by monotonic time. A stale observation cannot
   extend an event.
4. After face loss, strong pose, occlusion, or a discontinuous timestamp, FSMs
   enter recovery and require fresh confirmation.
5. Monitoring-unavailable is a separate system state, not evidence that the
   driver is absent, distracted, or drowsy.

## Recommended thresholds

| Metric/event | Recommended development policy | Rationale/trade-off |
| --- | --- | --- |
| Eye calibration | Require 2 s stable, forward, high-quality open-eye evidence; recalibrate after a new driver/session, absence over 1 s, or sustained invalid geometry over 2 s | Reduces fixed-provider bias; delays availability briefly |
| Eye close/reopen | Normalized openness close <=0.25, reopen >=0.40 | Retains hysteresis; must be recalibrated with annotations |
| Ordinary blink | Closure 80–700 ms plus 80 ms confirmed reopening | Raises current 500 ms ceiling while rejecting noise |
| Long blink | Closure >700 ms and <1500 ms | Do not silently discard; count separately as fatigue evidence |
| Prolonged closure | Continuous visible closure >=1500 ms | Immediate high-severity drowsiness evidence |
| Blink refractory | 150 ms after confirmed reopening | Prevents one blink being split by landmark jitter |
| PERCLOS | 60 s rolling window; >=80% known coverage; warning >=20%; drowsy >=35% | Existing conservative values; delayed validity is explicit |
| Yawn | Open >=0.65, close <=0.45, 120 ms confirmation, >=800 ms duration, 2 s refractory | Preserves current values; add pose/mouth-quality gate before use |
| Head zones | Yaw enter +/-25°, exit +/-15°; pitch enter +/-18°, exit +/-10°; 150 ms confirmation | Hysteresis avoids boundary jitter |
| Head count | Count one confirmed neutral-to-direction entry; rearm only after confirmed neutral return | Prevents repeated counts while holding a pose |
| Gaze zones | Horizontal enter +/-0.20, exit +/-0.12; vertical enter +/-0.25, exit +/-0.15; 150 ms confirmation | Supported by current visible development traces; IR remains gated |
| Distraction | Continuous confirmed away evidence for 2 s; clear after 500 ms forward evidence | Balances brief mirror/instrument glances against delayed warning |
| Presence | Present after 300 ms; absent after 1 s; evaluate only inside configured driver-seat ROI | Avoids flicker and faces outside the driver position |
| Occlusion/unavailable | Record after 500 ms; show monitoring unavailable after 2 s; never convert to eye closure | Separates sensor limitation from driver state |
| Combined warning | PERCLOS >=20% or two yawns within 60 s | Low-severity warning evidence |
| Combined drowsy | PERCLOS >=35%, prolonged closure, or product-approved equivalent evidence | High-severity driver alert |
| Alert recovery | Retain warning at least 5 s; clear only after 10 s valid normal evidence | Current 3 s recovery is too short for HMI stability |

The deliberate-blink development clip contains 14 closures around 0.5–1.0 s.
Under this policy, shorter closures can become ordinary blinks and longer ones
become explicit long blinks rather than disappearing. Exact boundaries remain
subject to annotated count-error and delay evaluation.

## Calibration and reacquisition

- Calibration uses only high-quality, forward, present-driver observations.
- It must never learn while the eyes are closed, the face is strongly posed, or
  the ROI is occluded.
- Startup calibration may run silently, but the UI shows `CALIBRATING` rather
  than a misleading normal state.
- Reacquisition is automatic after temporary loss. It must not require a button
  press while driving.
- Persisted biometric identity is not required. If recognition is enabled later,
  its enrollment/retention policy remains separate from DMS calibration.

## Display, count, record, and alert policy

`Record` means an anonymous timestamped event/metric, not video or identity.
Continuous raw video retention is outside this policy.

| Output | Display | Count | Record | Alert |
| --- | --- | --- | --- | --- |
| Eye openness/EAR | Diagnostic or service UI | No | Sampled aggregate only | No |
| Ordinary blink | Optional count | Yes | Event | No |
| Long blink | Yes | Separate count | Event and duration | Low warning evidence |
| Prolonged closure | Yes | Yes | Event and duration | Immediate high alert |
| PERCLOS | Yes when coverage valid | No | Periodic aggregate | Warning/drowsy thresholds |
| Yawn | Yes | Yes | Event and duration | Only through combined policy |
| Head direction | Yes | Per direction | Entry/return event | No standalone alert |
| Gaze direction | Optional; suppress when unknown | No | Away episodes | No standalone alert |
| Distraction | Yes | Episode count | Episode and duration | Alert after 2 s |
| Driver presence | Yes | Transition count | State transition | Alert when driving and absent |
| Eye/face occlusion | `MONITORING UNAVAILABLE` | Episode count | Cause/confidence/duration | Availability warning after 2 s |
| Combined drowsiness | Yes | State transitions | Evidence and state | Warning/high alert |

## Alert priority and coexistence

1. System/camera unavailable or driver absent.
2. Prolonged closure / combined drowsy.
3. Sustained distraction.
4. Combined warning / long-blink evidence.
5. Informational counts and diagnostics.

Only the highest active priority should drive the primary HMI alert. Lower
events remain logged so competing audible warnings do not stack.

## Approved decisions

The product owner approved the proposal as written on 2026-08-28, including:

1. the ordinary/long/prolonged closure boundaries of 700/1500 ms;
2. PERCLOS 60 s, 80% coverage, 20% warning, and 35% drowsy values;
3. yawn 800 ms and two-yawns-in-60-s combined-warning policy;
4. 2 s distraction and monitoring-unavailable delays;
5. 25° yaw and 18° pitch entry zones;
6. automatic calibration/reacquisition behavior;
7. the display/count/record/alert matrix;
8. separation of recognition/biometric retention from normal DMS operation;
9. anonymous event/aggregate retention duration and whether raw video is ever
   retained in the production product.

These values are now the approved Stage 20 operational policy. They must become
named configuration profiles rather than hard-coded provider behavior.
Annotated evaluation may propose a controlled revision only with before/after
evidence and a documented product-policy review.
