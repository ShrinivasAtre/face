# Stage 20 trace error and calibration analysis

Date: 2026-08-27

## Boundary

This report completes Task 3 on the current unannotated development data. It
uses 28,200 schema-4 observations from 34 private visible-light and IR clips.
Recordings, audio, per-frame traces, and the private clip-name mapping remain
outside Git. Anonymous IDs `C01`–`C34` are used here.

The recordings have spoken action cues but no timestamped ground truth.
Consequently, this report separates confirmed protocol failures from candidates
that Task 4 must annotate. It does not claim precision, recall, count error, or
detection delay.

## Reproduction

`scripts/analyze_stage20_traces.ps1` converts a directory of schema-4 CSV traces
into private `trace_intervals.csv`, `clip_summary.csv`,
`private_clip_mapping.csv`, and `analysis_totals.json` artifacts. The completed
run found 902 intervals: 139 eye-unknown runs, 297 eye-closed runs, 192 blink
events, 10 yawn events, 32 head events, 29 distraction events, and 56
face-missing runs. These are output counts, not ground-truth error counts.

## Confirmed failures

### Deliberate closures rejected as blinks

Dedicated blink clip `C22` has 331 closed-state frames in 14 runs and zero
temporal blink events:

| Frames | Time (ms) | Closed-run ms |
| --- | --- | ---: |
| 71–86 | 2399.508–2899.406 | 499.898 |
| 119–136 | 3999.180–4565.730 | 566.550 |
| 169–185 | 5665.505–6198.729 | 533.224 |
| 229–246 | 7665.095–8231.646 | 566.551 |
| 282–307 | 9431.400–10264.562 | 833.163 |
| 339–366 | 11331.010–12230.826 | 899.816 |
| 420–442 | 14030.456–14763.640 | 733.183 |
| 487–517 | 16263.332–17263.127 | 999.795 |
| 543–571 | 18129.616–19062.758 | 933.142 |
| 622–646 | 20762.409–21562.245 | 799.836 |
| 677–697 | 22595.367–23261.897 | 666.530 |
| 761–782 | 25394.793–26094.650 | 699.857 |
| 808–837 | 26961.138–27927.607 | 966.469 |
| 869–895 | 28994.055–29860.544 | 866.489 |

Closure is detected, so this is not landmark absence. The fixed 500 ms maximum
blink closure, including the subsequent reopen transition, rejects all 14.
Root cause: **FSM duration threshold**, with a secondary event-policy mismatch.

### Pose creates false yawn events

Head-pose-only clips produced yawn events at `C05` frame 458/15517.000 ms and
494/16734.019 ms, and `C06` frame 477/16550.702 ms. Mouth geometry is accepted
whenever facial geometry exists and lacks pose/reprojection or mouth-ROI quality
gating. Root cause: **quality-gate omission/provider sensitivity**; the yawn FSM
correctly counted the erroneous input.

### Neutral calibration creates head/distraction events

Neutral clip `C08` produced head events at frame/time 363/12125.859 ms,
390/13025.305 ms, and 783/26117.235 ms. It was distracted during frames 503–573
(16789.651–19121.547 ms; 2331.896 ms). Root cause: **neutral
calibration/reacquisition and fixed pose/gaze thresholds**, not the two-second
distraction timer.

## Quantified weaknesses requiring annotation

| Category | Evidence | Root-cause class |
| --- | --- | --- |
| Pose eye coverage | Four pose clips: 1,542 unknown frames/3,600 (42.83%) | Quality gate; safe suppression but poor coverage |
| IR natural distraction | `C18`: 2 events, 456 distracted frames (50.67%); principal runs 9160.724–12637.787 and 19525.047–29488.171 ms | Calibration/threshold candidate |
| Partial-eye closure ambiguity | `C10`, `C20`, `C34`: 189, 418, 166 prolonged-closure frames | Threshold/FSM policy candidate |
| Occlusion confusion | `C09`: 75 prolonged-closure frames; no reliable in-frame hand/object occlusion output | Provider/quality-model limitation |
| Blink inconsistency | Dedicated blink clips: 7 (`C01` visible), 3 (`C12` IR), 0 (`C22` visible) | Fixed calibration and duration policy |
| Natural blink spread | Baseline/natural clips range from 1/300 to 21/900 events | Calibration candidate; ground truth required |

Pose-related unknown frames are not false events: they show that the safety gate
prevents closure evidence at the cost of EAR/PERCLOS coverage. Partial-eye and
natural-activity candidates likewise cannot be scored until independent state
and visibility labels exist.

## Root-cause summary

- **FSM/threshold:** 14 confirmed deliberate closure runs rejected as blinks;
  partial-eye/prolonged-closure policy remains ambiguous.
- **Calibration/reacquisition:** one confirmed neutral distraction episode and
  three neutral head events; two additional IR-natural distraction candidates.
- **Quality gate:** 1,542 pose frames suppressed; the mouth path lacks an
  equivalent pose-quality gate and caused three confirmed false yawns.
- **Provider/model:** in-frame hand/object eye occlusion is not reliably
  classified and can be interpreted as closure.

## Decisions carried into Tasks 4 and 5

1. Annotate ordinary blink, deliberate long blink, prolonged closure, and eye
   closure separately before changing the 500 ms policy.
2. Evaluate session/subject eye calibration or an eye-openness model across
   visible and IR slices.
3. Add pose/reprojection and mouth-ROI labels before tuning yawn thresholds.
4. Define calibration/reacquisition after pose, absence, and occlusion.
5. Keep strong-pose input unknown, but make known-eye coverage an acceptance
   metric.
6. Evaluate a trained eye-ROI model for visibility, closed eye, partial
   occlusion, hand/object occlusion, blur, exposure, glasses, and IR.

Task 3 is complete for the present unannotated development dataset. Production
accuracy remains open pending Task 4 annotations.
