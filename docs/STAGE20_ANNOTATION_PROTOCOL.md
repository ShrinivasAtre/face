# Stage 20 annotation and scoring protocol

## Privacy boundary

Raw recordings, extracted frames, audio, identity-bearing clip names, and
annotations remain outside Git. Commit only this protocol, synthetic examples,
tooling, anonymous aggregate metrics, and approved checksums. Use stable
anonymous `subject_id`, `session_id`, and `clip_id` values. Keep the private
mapping separately.

## Timeline and labels

Use video presentation time in milliseconds. Every interval is half-open:
`[start_ms,end_ms)`. Annotate uncertain boundaries with `boundary_tolerance_ms`
and `label_quality=uncertain`; exclude uncertain labels from the primary score
and report them separately.

Required interval labels are:

- `face_present`, `face_absent`;
- `eye_visible`, `eye_partial`, `eye_occluded`, `eye_closed`, `eye_open`;
- `blink`, `long_blink`, `prolonged_closure`;
- `yawn`;
- `head_left`, `head_right`, `head_up`, `head_down`, `head_neutral`;
- `gaze_left`, `gaze_right`, `gaze_up`, `gaze_down`, `gaze_forward`;
- `distracted`;
- `occlusion_hand`, `occlusion_object`, `occlusion_glasses`,
  `occlusion_other`.

For eye labels, annotate the right and left eye independently using `side`, then
add a binocular event label where required. Eye state and visibility are
orthogonal: a closed but visible eye is not occluded. A hand covering a closed
eye is occluded with eye state unknown.

## Event definitions

- A blink begins at the first visible closing movement and ends at stable
  reopening. `long_blink` is annotated separately from ordinary blink; the
  operational duration boundary remains a Task 6 decision.
- Prolonged closure begins when continuous visible closure crosses the annotated
  product boundary; retain the physical closure-start timestamp as an attribute.
- A yawn begins with sustained mouth opening associated with a yawn and ends
  after closure. Speech, smiling, and pose distortion are negative examples.
- Head movement is one neutral-to-direction-to-neutral excursion. Record both
  directional entry and return boundaries.
- Distraction is annotated from driver attention evidence, not merely head or
  gaze threshold crossing. Record its observable cause when known.
- Presence is based on the driver seat ROI, not any face elsewhere in frame.

## Annotation workflow

1. Preserve the source clip and calculate a checksum; never transcode the
   scoring copy after annotation starts.
2. First pass: label visibility, presence, and unusable spans without viewing
   model output.
3. Second pass: label eye state/events, mouth/yawn, pose, gaze, and distraction.
4. A second annotator independently labels the final test subset.
5. Adjudicate disagreements greater than two frames or the declared boundary
   tolerance. Retain both original labels and the adjudicated result.
6. Freeze calibration/development labels before examining final-test scores.

## CSV contract

Required columns:

```text
clip_id,subject_id,session_id,event,side,start_ms,end_ms,label_quality,boundary_tolerance_ms,annotator_id,notes
```

`notes` must not contain names or other identity data. Prediction CSVs use:

```text
clip_id,event,side,start_ms,end_ms,confidence
```

## Metrics

- Event matching is one-to-one within the same clip, event, and side. A match
  requires interval overlap or start-time difference within the configured
  tolerance.
- Report TP, FP, FN, precision, recall, F1, signed and absolute count error per
  clip, onset delay, offset delay, and absolute onset delay percentiles.
- For state labels, additionally report duration-weighted confusion and known
  coverage. Unknown predictions do not become false negatives silently; report
  coverage and conditional accuracy together.
- Report every metric by visible/IR, subject, glasses, lighting, pose, distance,
  and occlusion slice. Never tune and report on the same subject/session split.
- Report false alarms per hour for distraction, yawn, prolonged closure, and
  presence loss.

`scripts/score_stage20_annotations.ps1` provides deterministic event matching,
count error, onset/offset delay, and absolute onset-delay percentiles.
`scripts/score_stage20_states.ps1` provides duration-weighted known coverage and
conditional state accuracy, keeping unknown output explicit.
`scripts/aggregate_stage20_scores.ps1` produces anonymous micro-aggregates by
metadata slice. Synthetic fixtures in `demo/` exercise all three tools without
using personal data. `scripts/validate_stage20_annotations.ps1` checks the CSV
contract, interval bounds, label quality, tolerance, and duplicates before an
annotation file can be scored. `scripts/score_stage20_agreement.ps1` compares
exactly two independent annotators, reports event F1 and boundary differences,
and identifies groups requiring adjudication. These CSV interfaces are the
tool-neutral import/export boundary for a spreadsheet or video annotation UI.
Step-by-step instructions for a human operator are in
`docs/STAGE20_TIMESTAMPING_OPERATOR_GUIDE.md`.
