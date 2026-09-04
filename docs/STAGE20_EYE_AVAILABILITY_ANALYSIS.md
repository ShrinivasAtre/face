# Stage 20 eye availability and model-gate analysis

Date: 2026-09-03

## Scope

This analysis uses the adjudicated dense eye truth and unchanged schema-5
traces. Private rows remain outside Git. It diagnoses availability without
changing the approved policy thresholds and without training a model.

## Availability result

Of 1,216 evaluable paired samples, 1,068 have a model open/closed state. Every
one of the 148 model-unknown samples is explicitly classified by the runtime as
either `recovering` or `low_confidence`:

| Slice | Reason | Samples | Episodes | Duration |
|---|---|---:|---:|---:|
| subject A visible | recovering | 40 | 4 | 8.00 s |
| subject A visible | low confidence | 27 | 2 | 5.40 s |
| subject B IR | recovering | 30 | 1 | 6.02 s |
| subject B visible | recovering | 47 | 3 | 9.41 s |
| subject B visible | low confidence | 4 | 2 | 0.80 s |

All these samples retain 100% face detection, semantic-landmark validity,
crop visibility, and recorded image-quality confidence. The low-confidence
episodes reach absolute yaw of 53.0 degrees and 36.6 degrees respectively,
consistent with the intentional pose-quality gate. The IR loss is one startup
calibration episode, not an observed IR open/closed classification error.

When usable, the model is known for every evaluable sample. Conditional
classification errors comprise 9 closed-to-open and 7 open-to-closed samples;
there are no conditional errors in the IR slice.

## Decision

Do not loosen the approved visibility, pose, calibration, or PERCLOS thresholds
from this small development set. The conservative unknown behavior is operating
as designed and prevents invalid observations from becoming normal/open.

The dedicated eye-ROI benchmark gate is **triggered but not executable yet**:

- overall model-known coverage is 87.82%, and the sole IR session is 78.10%;
- explicit eye-occlusion event recall remains zero in the six-clip event score;
- only two subjects and one IR subject/session are available;
- the current crop set was previously rejected as training-ready; and
- model training has not been authorized.

An eye-ROI model benchmark may begin only after the second private batch adds
subject/session-disjoint IR, clear-glasses, occlusion, and long-window coverage,
the crop audit passes, and the product owner separately authorizes training.
The benchmark contract remains the classifier-only versus multi-task openness
comparison in `docs/STAGE20_EYE_OCCLUSION_MODEL_OPTIONS.md`.

## Reproducibility

`scripts/score_dense_eye_roi_labels.ps1` emits private sample and window files
plus anonymous state, confusion, availability, and PERCLOS summaries. The
availability summary reports episode count, detected/semantic validity,
quality/visibility/contrast aggregates, and maximum absolute pose by anonymous
slice. The committed test covers adjudication, combined state, confusion,
availability, duration weighting, and a known 60-second PERCLOS result.
