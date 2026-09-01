# Stage 20 annotated accuracy checkpoint

Date: 2026-09-01

## Scope

This is an anonymous development checkpoint over six adjudicated clips, two
subjects, three session/lighting slices, 8,501 frames, and approximately 283.5
seconds. It is useful for finding algorithm and annotation gaps; it is not a
production, regulatory, subgroup, or final-test accuracy claim. Private clips,
annotations, predictions, traces, and mappings remain outside Git.

## Event results

| Event | Truth | Predicted | Precision | Recall | F1 | Absolute count error |
|---|---:|---:|---:|---:|---:|---:|
| blink | 72 | 62 | 0.871 | 0.750 | 0.806 | 26 |
| long blink | 15 | 12 | 0.750 | 0.600 | 0.667 | 9 |
| prolonged closure | 4 | 2 | 0.500 | 0.250 | 0.333 | 4 |
| yawn | 4 | 5 | 0.800 | 1.000 | 0.889 | 1 |
| eye occluded | 5 | 0 | 0.000 | 0.000 | 0.000 | 5 |
| gaze left | 10 | 11 | 0.727 | 0.800 | 0.762 | 5 |
| gaze right | 9 | 9 | 0.889 | 0.889 | 0.889 | 2 |
| gaze up | 5 | 8 | 0.000 | 0.000 | 0.000 | 13 |
| gaze down | 6 | 2 | 0.000 | 0.000 | 0.000 | 8 |
| head left | 3 | 2 | 0.000 | 0.000 | 0.000 | 5 |
| head right | 1 | 4 | 0.000 | 0.000 | 0.000 | 5 |
| head up | 2 | 0 | 0.000 | 0.000 | 0.000 | 2 |
| head down | 1 | 1 | 0.000 | 0.000 | 0.000 | 2 |

Absolute count error is summed at the clip/event/side level, so it can exceed
the absolute difference of aggregate truth and prediction counts.

The single yawn false positive corresponds to approximately 12.7 false alarms
per hour when naively scaled from this short set. That rate is statistically
unstable and is reported only as a risk indicator. Presence false-alarm rates
cannot be interpreted because absence/presence truth is incomplete in five
clips.

## Findings

1. Ordinary blink performance is promising but still misses 18 of 72 annotated
   events. Long-blink recall is 0.60 and prolonged-closure recall is 0.25, so
   fatigue-relevant eye-event sensitivity does not pass a production gate.
2. Explicit in-frame eye occlusion has zero recall. The current heuristic and
   landmark availability cannot solve the observed hand/object occlusion case;
   this confirms the eye-ROI model/data route, subject to the separate gate.
3. Yawn is the strongest current non-eye event on this set, but four truth
   events and one false positive are too few for threshold acceptance.
4. Horizontal gaze is directionally useful. Vertical gaze has zero matched
   events, indicating provider calibration/sign/scale or annotation-contract
   mismatch that must be localized before threshold tuning.
5. Head-direction event matching is zero despite predicted counts. The current
   neutral-entry event representation and annotated direction intervals are not
   aligned closely enough for evaluation; trace localization is required before
   deciding whether the defect is estimator calibration, FSM timing, or scoring
   contract.
6. Dense `eye_open`, `eye_closed`, and visibility intervals are absent from the
   adjudicated file. Therefore duration-weighted eye-state accuracy, known-eye
   coverage, openness error, and PERCLOS error cannot yet be scored.
7. The dataset is not subject/session rich enough to tune and then report an
   independent result. No numerical policy change is justified from this set.

## Decision

The Stage 20 mechanism/policy implementation remains accepted, but its
production-accuracy gate remains open. Keep the approved thresholds unchanged
until a larger subject/session-disjoint dataset supplies dense eye state and
visibility truth. Prioritize:

1. dense eye-ROI/eye-state review using
   `docs/STAGE20_EYE_ROI_LABELING_PROTOCOL.md`;
2. trace-level localization of vertical-gaze and head-event boundary failures;
3. timestamped dashcam yawn, inattention, obstruction, presence, and monitoring
   availability intervals;
4. more subjects plus repeated visible/IR/glasses/low-light sessions; and
5. repeat scoring on a frozen development split before seeking any threshold or
   model-training approval.
