# Stage 20 eye-ROI crop readiness audit

Date: 2026-09-01

## Scope and privacy

The product owner authorized local extraction and assessment of eye-region
crops from the private development recordings, but did not authorize model
training. The crop images, source mappings, contact sheet, per-crop manifest,
and detailed annotations remain outside Git. This document contains anonymous
aggregate findings only.

The audit used `scripts/audit_eye_roi_crops.py` against the private candidate
manifest. It verifies every image, reports anonymous image-property and class
aggregates, and can generate a private balanced contact sheet for visual
review. The tool does not convert candidate labels into ground truth.

## Extraction integrity

- Six timestamped clips from two subjects were sampled every six frames.
- 1,414 left/right pairs (2,828 PNG crops) were present.
- All crops decoded successfully at the expected 128x80 dimensions.
- No manifest entry was missing and no extraction failure was recorded.
- Left and right counts were exactly balanced at 1,414 each.

## Candidate-label balance

| Timestamp-derived candidate class | Crops | Share |
|---|---:|---:|
| unlabelled visible | 2,133 | 75.42% |
| blink | 207 | 7.32% |
| prolonged closure | 196 | 6.93% |
| long blink | 150 | 5.30% |
| eye occluded | 142 | 5.02% |

One visible head/gaze clip contributes 1,062 crops (37.55% of the complete
set). Only 290 crops (10.25%) come from IR, all from one subject/session and one
blink/closure scenario. The candidate inventory has no explicit crop-level
classes for partial visibility, clear/dark glasses, invalid quality, blur,
under/over-exposure, or hard-negative hand/object-near-eye examples.

## Image-property findings

- All crops have non-degenerate contrast and no material black/white clipping
  under the audit's conservative descriptive screen.
- Median grayscale brightness differs by side: 97.56 for left crops and 118.28
  for right crops. Median contrast is 31.12 versus 39.53. Side normalization and
  augmentation are therefore required, and random left/right splitting could
  create avoidable leakage or shortcut learning.
- The IR clip has lower median contrast and spatial gradient than most visible
  clips. It is a genuine domain slice, not sufficient IR diversity.
- The occlusion candidate class is materially brighter and lower-gradient than
  the overall set. A model could learn recording/lighting shortcuts unless
  occlusion and non-occlusion examples are balanced within subjects, sessions,
  lighting domains, and occluder types.

Gradient and contrast are descriptive diagnostics only. They are not used as
blur or visibility truth labels.

## Private visual-review findings

A deterministic balanced contact sheet sampled 12 crops from each candidate
class. The review confirmed:

- blink, long-blink, and prolonged-closure candidate intervals contain both
  visibly open and visibly closed eyes;
- the prolonged-closure sample is especially impure because event-level
  boundaries are not equivalent to dense frame-level closure labels;
- most eye-occlusion candidates contain a real hand/object obstruction, but
  boundary samples and eye-ROI localization vary, including visible-eye and
  no-eye crops;
- crop alignment varies with pose and obstruction; and
- unlabelled-visible candidates contain open, closed, and transitional eyes.

These are expected consequences of expanding interval annotations to every
sampled frame. The candidate labels are useful for building a review queue, but
are not safe training targets.

## Leakage-safe dataset decision

This crop set is **not sufficient to authorize or execute model training**:

1. Two subjects cannot support credible subject-disjoint train, validation,
   and final-test partitions.
2. Frames sampled from the same event and video are strongly correlated. Eye
   sides, neighboring frames, events, sessions, and subjects must stay in the
   same partition; random crop splitting is prohibited.
3. Required semantic classes are missing or noisy at crop level.
4. IR, glasses, low-light, pose, distance, partial-eye, and occluder diversity
   are insufficient.
5. Candidate-class distribution and recording domains permit shortcut learning.

The current set remains valuable for extractor QA, annotation-tool development,
hard-case discovery, and a future pretrained inference-only feasibility check.

## Required next data gate

Before requesting training approval:

- obtain at least three disjoint subject groups for train/validation/test, with
  materially broader diversity preferred for any production claim;
- densely review sampled crops into `visible_open`, `visible_closed`,
  `partial`, `occluded_hand_or_object`, `glasses_degraded`, and
  `invalid_quality`;
- retain an `uncertain`/exclude disposition rather than forcing ambiguous
  frames into a class;
- add visible-light and IR examples of every safety-relevant class, plus clear
  and dark glasses, pose, distance, low light, blur/exposure failures, and hard
  negatives;
- group the split by subject and session before sampling or augmentation; and
- freeze an untouched final test partition before any threshold or model
  selection.

No training approval is requested from this audit. A separate explicit request
will be made only after the label inventory and leakage-safe split satisfy this
gate.
