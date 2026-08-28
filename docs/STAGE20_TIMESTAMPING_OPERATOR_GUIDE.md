# Stage 20 timestamp annotation operator guide

## What the annotator receives

- Private video files and their checksums.
- A private clip-to-anonymous-ID mapping (`C01`, `C02`, ...).
- A copy of the CSV header from `docs/STAGE20_ANNOTATION_PROTOCOL.md`.
- A video player/editor that shows millisecond time or frame number. The tool is
  optional; the exported CSV contract is authoritative.

Do not upload recordings, screenshots, extracted frames, names, or annotations
to GitHub. Do not place names in `notes`.

## Before starting

1. Work from the original, unmodified scoring copy.
2. Verify its checksum and frame rate.
3. Confirm that seeking uses the video's presentation timeline, not wall-clock
   time.
4. Assign a non-identifying `annotator_id`.
5. Create one CSV per annotation pass. Annotators must not see model predictions
   or each other's labels.

## Annotation passes

### Pass 1 — observability and presence

Watch at normal speed, then refine boundaries frame by frame. Label:

- `face_present` / `face_absent`;
- per-eye `eye_visible`, `eye_partial`, or `eye_occluded`;
- occluder type: hand, object, glasses, or other;
- spans that are too blurred, dark, bright, cropped, or strongly posed to judge.

Closed but clearly visible is `eye_visible` plus `eye_closed`, not occluded.

### Pass 2 — eye state and events

Label each eye independently as open/closed, then label binocular events:

- ordinary `blink`;
- `long_blink`;
- `prolonged_closure`.

Start at the first visible closing movement and end at stable reopening. Do not
force an uncertain interval: set `label_quality=uncertain` and record the
boundary tolerance.

### Pass 3 — mouth, head, gaze, and attention

Label yawns; neutral/left/right/up/down head zones; gaze zones; and distraction.
Speech and smiles are yawn negatives. A brief mirror or instrument glance is
away gaze but is not automatically distraction; label distraction from the
agreed operational definition.

## Timestamp mechanics

- Intervals are half-open: `[start_ms,end_ms)`.
- If the player shows frames, convert with the actual presentation timestamp;
  do not assume exactly 30 FPS when timestamps are available.
- For a 30 FPS approximation only, one frame is about 33.33 ms.
- Put the uncertainty of a hand-selected boundary in
  `boundary_tolerance_ms`; normally one or two frames.
- Adjacent intervals of the same label should be merged unless a genuine state
  change occurs.

Example:

```csv
clip_id,subject_id,session_id,event,side,start_ms,end_ms,label_quality,boundary_tolerance_ms,annotator_id,notes
C01,S01,session-01,blink,both,5233.3,5433.3,accepted,66.7,ann-a,
```

## Validation and handoff

From the repository PowerShell prompt:

```powershell
.\scripts\validate_stage20_annotations.ps1 `
  -Annotations D:\private-annotations\pass-a.csv `
  -OutputCsv D:\private-annotations\pass-a-errors.csv
```

For the final test subset, two people annotate independently. Combine their
rows into one private CSV and run:

```powershell
.\scripts\score_stage20_agreement.ps1 `
  -Annotations D:\private-annotations\two-annotators.csv `
  -OutputCsv D:\private-annotations\agreement.csv
```

Adjudicate every row marked `requires_adjudication=1` and every disagreement
larger than the declared tolerance. Preserve the two original files; write the
accepted result to a third CSV with `label_quality=adjudicated`.

## Minimum useful first batch

Annotate six short clips before attempting the whole collection:

1. visible neutral;
2. visible deliberate blinks/closures;
3. visible yawn and speech/smile negatives;
4. visible head/gaze movement;
5. IR eye closure/blink;
6. visible or IR hand/object eye occlusion.

Two annotators should independently label at least clips 2, 3, and 6. Return
the validated/adjudicated CSVs plus source checksums. No video needs to be
copied into the repository.
