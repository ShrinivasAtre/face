# Stage 18 DMS observation and scheduling core

The `dms_core` CMake target contains no model-provider or image-processing dependency. It defines the information and scheduling boundary used by later DMS metrics, FSMs and provider adapters.

## Observation contract

Every observation carries:

- a source-frame ID and monotonic capture timestamp;
- the monotonic production timestamp and source age at production;
- explicit source validity (`Missing`, `Valid`, or `Occluded`);
- confidence and visibility in the normalized `[0, 1]` range;
- a semantic value such as eye contours, face geometry, presence, recognition, or associated object information.

Consumers classify an observation at their current monotonic time. Precedence is missing, occluded, future timestamp, stale, low confidence/visibility, then usable. Missing and occluded are never silently interpreted as negative/open/absent states.

`ObservationQualityGate` applies loss immediately and requires a configurable
sustained usable interval at startup and after reacquisition. During that
interval the observation is `Recovering`, so an occlusion boundary cannot become
a blink, yawn, pose, gaze, or drowsiness event. The gate consumes provider-
neutral confidence, visibility, and validity; a provider reporting landmarks is
not by itself proof that the eyes are unoccluded.

Provider-specific landmark indices belong only in provider adapters. They are prohibited from this core and from downstream metric/FSM code.

## Scheduling contract

`SchedulerConfig` gives eye, facial geometry, recognition and object work independent cadences, maximum observation ages, confidence/visibility thresholds, and optional uncertainty-triggered execution. Defaults are initial engineering values rather than product thresholds and remain replaceable configuration.

`CadenceScheduler` consumes caller-supplied monotonic timestamps; it never reads wall-clock time. First execution, elapsed cadence, uncertainty, not-due state and non-monotonic time are distinct outcomes.

`LatestFrameSlot<T>` has a strict depth of one. Publishing while work is pending atomically replaces the older frame and increments `superseded`; consumers can therefore become slower than capture without creating an unbounded queue or increasing source-frame age indefinitely.

The deterministic test replays a fixed timestamp sequence, proves independent task counts, checks all observation quality states, and verifies that three publications result in one latest frame, two superseded frames and a zero-depth slot after consumption.

## Temporal event contract

`DmsTemporalEvents` supplies provider-neutral, monotonic-time FSMs for yawn,
head-pose movement/counting, gaze/distraction, driver presence, and combined
drowsiness. Inputs are semantic quantities (normalized mouth opening, yaw/pitch,
normalized gaze, presence, eye metrics, and quality), never provider landmark
indices.

- Yawns require confirmed opening, a minimum duration, and confirmed closure,
  and count once.
- Head movements use enter/exit hysteresis, temporal confirmation, and require a
  neutral return before another directional count.
- Gaze uses hysteresis and confirmation; distraction combines sustained gaze or
  head deviation and requires confirmed forward recovery.
- Presence independently debounces present and absent evidence. Unknown quality
  remains unknown and is not silently treated as present.
- Drowsiness combines prolonged eye closure, available PERCLOS, and recent yawn
  evidence. It reports unknown when observation quality or presence is unknown,
  and debounces recovery from warning/drowsy states.

All defaults are preliminary engineering values. They are configurable and must
pass the Stage 20 product-definition and dataset calibration gate before release.
