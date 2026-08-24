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

Provider-specific landmark indices belong only in provider adapters. They are prohibited from this core and from downstream metric/FSM code.

## Scheduling contract

`SchedulerConfig` gives eye, facial geometry, recognition and object work independent cadences, maximum observation ages, confidence/visibility thresholds, and optional uncertainty-triggered execution. Defaults are initial engineering values rather than product thresholds and remain replaceable configuration.

`CadenceScheduler` consumes caller-supplied monotonic timestamps; it never reads wall-clock time. First execution, elapsed cadence, uncertainty, not-due state and non-monotonic time are distinct outcomes.

`LatestFrameSlot<T>` has a strict depth of one. Publishing while work is pending atomically replaces the older frame and increments `superseded`; consumers can therefore become slower than capture without creating an unbounded queue or increasing source-frame age indefinitely.

The deterministic test replays a fixed timestamp sequence, proves independent task counts, checks all observation quality states, and verifies that three publications result in one latest frame, two superseded frames and a zero-depth slot after consumption.
