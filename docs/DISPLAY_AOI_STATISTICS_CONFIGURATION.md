# Display, area-of-interest and statistics configuration

Date: 2026-09-03

## Boundary

Presentation selection and processing selection are separate. Hiding a value
must not disable the algorithm that supplies alerts or recorded evidence.
Likewise, a display crop must not silently become a processing crop.

## Proposed configuration groups

    display
      enabled
      show_video
      show_face_box
      show_eye_boxes
      show_mouth_box
      show_landmarks
      show_quality
      show_calibration
      show_presence
      show_eye_openness
      show_perclos
      show_blink_counts
      show_yawn_counts
      show_pose
      show_gaze
      show_drowsiness
      show_performance

    processing_roi
      enabled
      coordinate_space = normalized
      x, y, width, height
      minimum_face_coverage
      outside_policy = ignore

    statistics
      reset_on_confirmed_driver_change
      reset_on_session_start
      rolling_window_seconds
      minimum_known_coverage
      cumulative_enabled
      rolling_enabled

The implemented schema-1 file is a dependency-free UTF-8 `key=value` format
with dotted keys. It permits blank lines and full-line `#` comments. The loader
requires `schema_version`, rejects duplicate and unknown keys, accepts only
canonical `true`/`false` booleans, and applies the same semantic validation as
the in-memory value object. Omitted recognized settings retain documented safe
defaults. See `config/dms-presentation.example.conf`.

## Area-of-interest rules

1. Coordinates are normalized [0,1] values relative to the decoded frame, so
   configuration is independent of camera resolution.
2. The ROI is validated at startup. Non-finite, negative, zero-sized or
   out-of-frame rectangles fail closed with an actionable error.
3. The detector may process only the configured driver-seat ROI, but reported
   face and landmark coordinates are transformed back into full-frame space.
4. A face must satisfy a configurable coverage rule; a marginal/outside face is
   not the driver.
5. ROI loss becomes presence/monitoring Unknown or Absent through the existing
   quality and temporal contracts, never immediate fatigue.
6. Display-only crops and processing ROIs use distinct names and cannot alias
   the same configuration key.
7. Configuration and effective ROI are recorded in benchmark metadata without
   recording private video.

## Statistics contracts

- **Cumulative eye-open percentage:** known-open duration divided by known eye
  duration since the current statistics epoch. Unknown time is excluded from
  the numerator and denominator and reported as separate coverage.
- **Rolling eye-open percentage:** the same duration-weighted calculation over
  the configured trailing window.
- **Cumulative blink count:** accepted blink events since the current epoch.
- **Rolling blink count:** accepted event timestamps within the configured
  trailing window.
- **Reset:** occurs only on an explicit session reset or confirmed driver change
  according to approved policy; transient face loss does not erase statistics.
- **Insufficient coverage:** rolling percentages are unavailable when known
  coverage is below the configured minimum.

The existing PERCLOS implementation already supplies duration-weighted closed
fraction over a configurable window with a known-coverage gate. Eye-open
percentage is its complementary known-time statistic, not simply 100 minus
PERCLOS when coverage or classification domains differ.

## Safe implementation sequence

1. **Implemented on `feature/stage24-display-aoi-statistics`:** add a
   provider-neutral configuration value object, fail-closed validation,
   normalized-to-pixel ROI conversion and deterministic tests.
2. **Implemented:** add a deterministic dependency-free configuration loader
   with mandatory schema versioning and strict key/value validation.
3. **Implemented for the existing sponsor-demo renderer:** load an optional
   `--presentation-config` file and apply display enablement, video background,
   face box, metric groups and performance flags only while rendering. Detection,
   traces, alert FSMs and benchmark results remain independent of visibility.
   Eye and mouth box flags are reserved until those renderers exist.
4. **Implemented:** apply the normalized processing ROI before backend face
   detection, enforce the configured face-coverage gate, and restore accepted
   face boxes and landmarks to full-frame coordinates before semantic mapping,
   quality assessment, traces and rendering. Rejected marginal detections are
   represented as absent rather than fatigue.
   Disabled ROI preserves the prior full-frame detection behavior. Benchmark
   schema 7 records the configured normalized ROI, coverage requirement and
   effective pixel rectangle without recording private imagery.
5. Add cumulative/rolling statistics using monotonic intervals and event
   timestamps.
6. Add recorded-sequence tests for missing data, face loss, driver change,
   window trimming and non-monotonic time.
7. Present proposed defaults and UI wording for approval before product use.

## Stage 24 objective and acceptance criteria

Stage 24 makes presentation selection, driver-seat processing ROI and the
sponsor-requested eye/blink statistics configurable without coupling those
contracts to OpenCV, a camera backend or an operating system.

The first increment is accepted when default configuration validates, invalid
schema/ROI/statistics values fail closed with actionable errors, a disabled ROI
maps to the full frame, enabled normalized ROIs convert deterministically at
frame boundaries, and the behavior is covered by a registered C++ test. This
increment does not activate ROI processing or alter approved monitoring
thresholds.
