# Stage 20 dense eye-ROI labeling protocol

## Purpose and privacy

This protocol converts timestamp-derived crop candidates into human-reviewed
frame-level labels suitable for a future eye-ROI benchmark. It does not
authorize training. Private crops, manifests, labels, reviewer contact sheets,
and source mappings remain outside Git. Only the protocol, validator, synthetic
fixtures, and anonymous aggregates are committed.

## Label contract

Use one row per eye crop:

```text
clip_id,frame,timestamp_ms,side,visibility,eye_state,occluder,quality,annotator_id,notes
```

Allowed values:

- `side`: `left`, `right`
- `visibility`: `visible`, `partial`, `occluded`, `invalid`, `uncertain`
- `eye_state`: `open`, `closed`, `transition`, `unknown`
- `occluder`: `none`, `hand`, `object`, `glasses`, `other`, `unknown`
- `quality`: `accepted`, `uncertain`, `exclude`

Eye state and visibility are orthogonal. A visible closed eye is not occluded.
When visibility prevents an eye-state decision, use `eye_state=unknown`. Clear
glasses over a fully observable eye may be `visible` with `occluder=glasses`;
reflections or frames that prevent a decision become `partial`, `occluded`, or
`invalid` as appropriate.

Do not include names or other identity data in notes or annotator identifiers.

## Review workflow

1. Freeze source-video and crop-manifest checksums.
2. Split work by complete subject/session groups, never random crops.
3. Review crops without model predictions. Use neighboring frames only to
   resolve a transition, not to infer an eye hidden in the target crop.
4. Mark ambiguous boundaries `quality=uncertain`; mark unusable localization or
   corruption `quality=exclude` rather than forcing a semantic class.
5. Independently double-label the frozen final-test subjects/sessions.
6. Adjudicate visibility/state disagreements and retain both source label sets.
7. Freeze train/development/final-test groups before any model or threshold is
   selected.

Both eye sides, neighboring samples, and every event from a subject/session
must remain in the same partition. Augmented variants inherit the source split.

## Validation

Run:

```text
python scripts/validate_eye_roi_labels.py private-labels.csv --manifest private-candidate-manifest.csv
```

The validator checks the schema, value vocabulary, unique crop keys, manifest
membership, timestamp agreement, annotator presence, and basic semantic
consistency. The committed synthetic files demonstrate the interface without
containing images or biometric data.

Validation is necessary but not sufficient: class balance, subject/session
separation, reviewer agreement, domain coverage, and label provenance remain
part of the future training-approval gate.
