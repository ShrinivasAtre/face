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

The eventual file syntax may be JSON, TOML or another dependency-free format.
Selection is an implementation decision only after parser, schema evolution and
deployment constraints are reviewed.

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

1. Add a provider-neutral configuration value object and validation tests.
2. Add a deterministic configuration loader with schema versioning.
3. Apply display flags only in rendering.
4. Apply processing ROI before face detection and transform results back to
   full-frame coordinates.
5. Add cumulative/rolling statistics using monotonic intervals and event
   timestamps.
6. Add recorded-sequence tests for missing data, face loss, driver change,
   window trimming and non-monotonic time.
7. Present proposed defaults and UI wording for approval before product use.

