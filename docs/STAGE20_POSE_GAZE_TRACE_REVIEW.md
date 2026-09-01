# Stage 20 pose and gaze trace review

Date: 2026-09-01

## Scope

Anonymous trace-level review of the six-clip adjudicated development checkpoint.
Private intervals, recordings, traces, and mappings remain outside Git.

## Head-pose findings

The pose estimator exposed OpenCV camera-coordinate Euler signs directly while
the provider-neutral FSM/UI convention defines negative yaw as driver-left and
negative pitch as driver-up. Recorded physical left/up intervals consequently
appeared as right/down.

The estimator now normalizes both signs at the semantic adapter boundary. A
3,186-frame rerun of the head/gaze clip produced:

- physical left event at 84.916 s inside annotated left 84.697-88.230 s;
- physical right event at 89.548 s inside annotated right 88.796-92.396 s;
- physical up event at 94.547 s inside annotated up 92.862-96.062 s.

This changes the first three relevant direction matches from zero to three
without changing the approved 25-degree yaw or 18-degree pitch thresholds.
One gentler up interval and the down interval have median pitch magnitude near
13-15 degrees and remain below the approved 18-degree entry threshold. Two
additional large-yaw predictions overlap gaze intervals that lack corresponding
head labels, showing an annotation-contract gap as well as a possible event
policy question. Threshold changes require broader evidence and product review.

## Gaze findings

Horizontal gaze separates well in the principal gaze clip: median normalized
horizontal values are approximately -0.32 to -0.36 for annotated left and
+0.38 to +0.45 for annotated right.

Vertical gaze does not reach the approved +/-0.25 entry threshold during normal
annotated movements. Typical medians are approximately -0.06 to -0.10 for up
and +0.07 to +0.11 for down. Conversely, eye closure can drive the iris ratio
to extreme vertical values and previously created false up/down events.

Gaze is now suppressed unless the calibrated temporal eye state is `Open`.
Trace review also exposed an FSM reacquisition defect: after a missing-quality
sample, the visible state was reset but the internal candidate was not, so the
same returning zone could remain `Unknown`. Head and gaze FSMs now reset their
candidate zone on loss, with deterministic reacquisition tests.

No vertical-gaze threshold change is accepted from this small set. The next
evaluation must rerun all six traces, regenerate predictions, and compare:

- closure-related vertical false events before/after the open-eye gate;
- horizontal gaze recall after missing/occluded periods;
- head direction matching after sign normalization; and
- gentle vertical head/gaze sensitivity across more subjects/sessions.

Windows focused tests, the private head/gaze rerun, and the full 23-test Release
suite passed. Ubuntu x64 and Orin aarch64 validation are still required before
this correction is considered cross-platform complete.
