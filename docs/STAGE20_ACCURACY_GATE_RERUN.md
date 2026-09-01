# Stage 20 pose/gaze correction accuracy rerun

Date: 2026-09-01

## Scope

This checkpoint reruns the six adjudicated development clips after semantic
head-pose sign normalization, head/gaze quality-loss reacquisition correction,
and suppression of iris gaze unless calibrated eye state is open. It uses
commit `536f1a4` on `feature/stage20-accuracy-gate` and the unchanged named
policy `stage20-approved-2026-08-28`.

Private clips, annotations, predictions, traces, mappings, and row-level scores
remain outside Git. The committed results are anonymous aggregates only.

## Reproducibility validation

- The adjudicated annotation CSV passed validation: 138 rows, six clips, one
  adjudicated annotator identity.
- All clips completed at their full decoded frame counts: 984, 993, 1,313,
  3,186, 868, and 1,157 frames, for 8,501 frames total.
- The committed trace-to-prediction converter reproduced all 126 rows of the
  retained pre-correction prediction CSV exactly before it was used on the new
  traces.
- Windows x64 Release passed 23 of 23 CTests.
- Orin aarch64 Release passed 23 of 23 CTests. The focused correction tests and
  benchmark were confirmed as ARM aarch64 binaries with no missing dependency.
- Ubuntu 24.04 x64 Release passed all 22 applicable CTests with the optional
  MediaPipe runtime disabled, matching the accepted Ubuntu validation path.

## Before/after event results

| Event | Old predicted | New predicted | Old TP/FP/FN | New TP/FP/FN | Old F1 | New F1 |
|---|---:|---:|---:|---:|---:|---:|
| blink | 62 | 63 | 54 / 8 / 18 | 54 / 9 / 18 | 0.806 | 0.800 |
| long blink | 12 | 12 | 9 / 3 / 6 | 9 / 3 / 6 | 0.667 | 0.667 |
| prolonged closure | 2 | 2 | 1 / 1 / 3 | 1 / 1 / 3 | 0.333 | 0.333 |
| yawn | 5 | 5 | 4 / 1 / 0 | 4 / 1 / 0 | 0.889 | 0.889 |
| gaze left | 11 | 13 | 8 / 3 / 2 | 8 / 5 / 2 | 0.762 | 0.696 |
| gaze right | 9 | 9 | 8 / 1 / 1 | 8 / 1 / 1 | 0.889 | 0.889 |
| gaze up | 8 | 0 | 0 / 8 / 5 | 0 / 0 / 5 | 0.000 | 0.000 |
| gaze down | 2 | 0 | 0 / 2 / 6 | 0 / 0 / 6 | 0.000 | 0.000 |
| head left | 2 | 5 | 0 / 2 / 3 | 1 / 4 / 2 | 0.000 | 0.250 |
| head right | 4 | 2 | 0 / 4 / 1 | 1 / 1 / 0 | 0.000 | 0.667 |
| head up | 0 | 1 | 0 / 0 / 2 | 1 / 0 / 1 | 0.000 | 0.667 |
| head down | 1 | 0 | 0 / 1 / 1 | 0 / 0 / 1 | 0.000 | 0.000 |
| eye occluded | 0 | 0 | 0 / 0 / 5 | 0 / 0 / 5 | 0.000 | 0.000 |

## Findings

1. Open-eye gating removed all ten previously predicted vertical-gaze events.
   Those predictions had no matches and included closure-driven excursions, so
   their removal improves safety even though vertical-gaze recall remains zero.
2. Semantic pose sign normalization changes the first physical left, right, and
   up intervals from zero matches to three matches without changing the approved
   yaw or pitch thresholds.
3. The remaining head false positives overlap incompletely labelled or
   differently labelled motion in the small gaze/head clip. This requires
   annotation-contract review and broader subjects/sessions before deciding
   whether it is an estimator or event-policy error.
4. Horizontal gaze-right performance is unchanged. Gaze-left retains the same
   recall but adds two unmatched episodes after reacquisition, reducing F1 from
   0.762 to 0.696. Trace/annotation review is required; threshold tuning is not
   justified from this set.
5. One additional unmatched ordinary blink reduces blink F1 from 0.806 to
   0.800. Fatigue-relevant long-blink and prolonged-closure results are
   unchanged and remain below a production gate.
6. Explicit eye occlusion remains at zero recall. Dense per-eye state and
   visibility truth is still required before openness, coverage, or PERCLOS
   accuracy can be claimed.

## Decision

The correction is accepted as a semantic/safety fix and is now cross-platform
validated. Stage 20 remains open for production accuracy. Keep the approved
threshold profile unchanged and proceed to dense per-eye state/visibility
annotation, adjudication, and state/PERCLOS scoring.

