# Stage 20 dense eye-state checkpoint

Date: 2026-09-03

## Scope

Two anonymous reviewers independently labelled 2,828 eye crops (1,414 paired
frames) from the six private development clips. A human adjudicator resolved
all 137 rows containing at least one disagreement. The source labels, crops,
adjudication decisions, per-sample results, and traces remain outside Git.

This checkpoint evaluates the unchanged `stage20-approved-2026-08-28` policy
and schema-5 traces. It does not train a model or change a threshold.

## Reproducibility and label quality

- All three files from each reviewer pass schema, vocabulary, crop membership,
  timestamp, uniqueness, and semantic validation.
- The adjudication overlay has 137 rows and 201 field decisions, with no
  missing, invalid, duplicate, or semantically inconsistent decision.
- Overall raw reviewer agreement was 98.94% for visibility, 97.84% for eye
  state, 97.98% for occluder, and 98.13% for quality.
- Cohen's kappa was 0.893 for visibility, 0.933 for eye state, 0.806 for
  occluder, and 0.331 for quality. The lower quality kappa occurs with 98.13%
  raw agreement and a strongly imbalanced class distribution.
- Private adjudication SHA-256:
  `AF4A22C86E4BCA4984242F5C7293F78F7BA49B5C1208FF44BB132D155AA09673`.

`scripts/score_dense_eye_roi_labels.ps1` deterministically verifies A/B source
provenance, applies only required adjudication decisions, combines left/right
truth conservatively, aligns sampled frames to schema-5 traces, and emits
private per-sample data plus anonymous summaries. Its synthetic test checks a
known disagreement, adjudication overlay, combined-eye state, and score.

## State-evaluation contract

A paired frame is evaluable only when both eye crops are `quality=accepted`,
both are `visible` or `partial`, and both states are `open` or `closed`.
Combined truth is closed if either eye is closed and open only if both are open.
Transition, unknown, excluded, invalid, occluded, and unresolved samples remain
unknown; they do not become correct or open by default.

Conditional accuracy measures correctness when the model reports open/closed.
End-to-end accuracy retains model-unknown samples as failures over evaluable
truth. Duration metrics weight each approximately six-frame sample by its
timestamp interval.

## Anonymous results

| Slice | Pairs | Truth evaluable | Model known / evaluable | Conditional accuracy | End-to-end accuracy |
|---|---:|---:|---:|---:|---:|
| subject A, visible baseline | 164 | 155 | 92.90% | 96.53% | 89.68% |
| subject A, visible blink | 166 | 151 | 93.38% | 99.29% | 92.72% |
| subject A, visible pose/gaze | 531 | 468 | 90.17% | 99.29% | 89.53% |
| subject B, IR closure | 145 | 137 | 78.10% | 100.00% | 78.10% |
| subject B, visible yawn | 219 | 210 | 82.86% | 97.13% | 80.48% |
| subject B, visible occlusion | 189 | 95 | 84.21% | 97.50% | 82.11% |
| **Overall** | **1,414** | **1,216** | **87.83%** | **98.50%** | **86.51%** |

Duration weighting produces essentially the same overall result: 85.76% of
sampled duration has evaluable human truth; model-known coverage over that
truth is 87.82%; conditional accuracy is 98.50%; and end-to-end accuracy is
86.51%.

The evaluable confusion counts are 927 open/open, 7 open/closed, 140
open/unknown, 125 closed/closed, 9 closed/open, and 8 closed/unknown. The
dominant failure is therefore availability rather than open/closed confusion.
IR has 78.10% model-known coverage despite perfect conditional classification,
and the explicit occlusion clip has only 50.26% evaluable human truth.

## Interpretation and next gate

This evidence supports conservative unknown-state handling and shows strong
classification when the calibrated temporal state is available. It does not
close the production gate: only two subjects and three sessions are represented,
IR covers one subject/session, and model-known coverage remains weakest on IR.
The aggregate closed fractions are diagnostic, not a fixed-window PERCLOS
accuracy claim.

Keep the approved thresholds unchanged. Next, add fixed-window truth PERCLOS
scoring with explicit human-known coverage, then determine whether the IR and
occlusion availability shortfall triggers the documented eye-ROI model
evaluation gate. Model training still requires separate approval.
