# DMS collaborator handoff

Updated: 2026-09-01

## Purpose and authority

This handoff lets a project collaborator reproduce tests, review timestamped
events, investigate gesture behavior, and provide evidence without needing the
private history of the primary development chat.

The product owner and primary development workflow retain authority for roadmap
changes, operational thresholds, privacy policy, production model selection,
training approval, releases, and merges. Collaborator experiments are valuable
evidence, not automatic product decisions.

## Read before acting

Read these files completely and follow them in this order:

1. `AGENTS.md`
2. `docs/DEVELOPMENT_PLAN.md`
3. `docs/NEXT_DEVELOPMENT_HANDOFF.md`
4. `docs/FOLLOW_ON_DEVELOPMENT_PLAN.md`
5. `docs/STAGE20_OPERATIONAL_POLICY_RECOMMENDATION.md`
6. `docs/STAGE20_ANNOTATION_PROTOCOL.md`
7. `docs/STAGE20_TIMESTAMPING_OPERATOR_GUIDE.md`
8. this handoff

Repository: `https://github.com/ShrinivasAtre/face`

## Repository checkpoints

- `main`: completed integration baseline; do not commit directly.
- `feature/sponsor-recorded-video-demo` at `a0f5dac`: sponsor-selected recorded
  video UI, statistics, Windows/Orin packaging, and validation evidence.
- `feature/stage20-eye-roi-readiness` at `423207e`: current Stage 20 accuracy
  checkpoint, local crop-readiness audit, dense eye-ROI labeling protocol,
  validator, and synthetic fixtures.

The Stage 20 branch may receive later pose/gaze correction commits. Always pull
and record the exact tested commit rather than relying only on this date.

## Overall development status

- Stages 1-16: complete CMake/C++ application and runtime-loaded MediaPipe
  integration.
- Stage 17: performance characterization complete. MediaPipe VIDEO mode is
  faster than the original path but remains CPU-bound at roughly 5-6 FPS on
  Orin. YuNet/LBF is responsive but is not the selected production stack.
- Stage 18: provider-neutral observations, timestamps, quality states,
  independent cadence, and bounded latest-frame scheduling complete on Windows,
  Ubuntu x64, and Orin.
- Stage 19: LBF/PFLD comparisons are preliminary. The tested PFLD candidate did
  not pass its replacement gate. Production geometry selection remains open.
- Stage 20: algorithms, named approved policy, recorded evaluation, annotation
  tooling, and deterministic FSM tests are implemented. The production-accuracy
  gate remains open.
- Stage 21: recognition and cigarette/hand-held-object work has not started and
  requires product/privacy decisions.
- Stage 22: final package matrix and Raspberry Pi validation remain pending.

## Architecture that must be preserved

- Native application code and runtime inference remain C++17 and CMake-built.
- DMS algorithms consume provider-neutral semantic observations, never raw
  MediaPipe/LBF/PFLD landmark indices.
- Model-specific topology stays inside adapters.
- Recognition, eye refinement, and object/context inference are independent
  components with independent cadence. An eye model is not required every
  frame.
- Unknown, missing, stale, low-confidence, and occluded observations never
  become open, closed, present, forward, or normal by default.
- Recorded event time comes from source-frame monotonic timestamps, not UI or
  inference completion time.

## Stage 20 implemented behavior

The named profile `stage20-approved-2026-08-28` defines:

- calibrated eye openness with hysteresis;
- ordinary blink, long blink, prolonged closure, and blink refractory behavior;
- rolling PERCLOS with a known-coverage gate;
- yawn, head movement/counting, gaze/distraction, presence, monitoring
  availability, and combined drowsiness FSMs;
- automatic calibration/reacquisition; and
- display/count/record/alert policy.

Thresholds are approved engineering defaults, not production-accuracy claims.
Do not change them from a small clip set without before/after evidence and
product-owner review.

## Current anonymous accuracy evidence

The current checkpoint covers six adjudicated development clips, two subjects,
three session/lighting slices, 8,501 frames, and about 283.5 seconds:

| Event | Precision | Recall | F1 |
|---|---:|---:|---:|
| ordinary blink | 0.871 | 0.750 | 0.806 |
| long blink | 0.750 | 0.600 | 0.667 |
| prolonged closure | 0.500 | 0.250 | 0.333 |
| yawn | 0.800 | 1.000 | 0.889 |
| eye occluded | 0.000 | 0.000 | 0.000 |
| gaze left | 0.727 | 0.800 | 0.762 |
| gaze right | 0.889 | 0.889 | 0.889 |

Vertical gaze and head-direction matching were initially zero. Trace review
found concrete sign, gating, reacquisition, threshold, and annotation-contract
issues. See `docs/STAGE20_ACCURACY_CHECKPOINT.md` and the latest pose/gaze review
before repeating those scores.

Dense open/closed/visibility state truth is not yet available, so eye-state
accuracy, known coverage, openness error, and PERCLOS error cannot be claimed.

## Eye-ROI benchmark status

Local extraction was explicitly approved; model training was not.

- 1,414 paired samples / 2,828 fixed 128x80 crops were extracted locally.
- All manifest files decoded at the expected dimensions with zero missing
  entries.
- The set is 75.42% unlabelled-visible and contains only two subjects.
- The sole IR slice is one subject/session and one closure scenario.
- Timestamp-derived blink/closure candidate intervals contain visibly open,
  closed, and transitional crops; they are review queues, not training truth.
- Partial, glasses, invalid-quality, hard-negative, and domain-balanced
  occlusion classes are insufficient.

This set is suitable for extractor QA and labeling-tool development, but not
for model training or production selection. Random crop splitting is
prohibited. Subject, session, event, neighboring frames, and both eye sides must
remain grouped.

Use:

```text
python scripts/audit_eye_roi_crops.py <private-candidate-manifest.csv> --output <private-audit.json> --contact-sheet <private-contact-sheet.png>
python scripts/validate_eye_roi_labels.py <private-labels.csv> --manifest <private-candidate-manifest.csv>
```

The labeling contract is in `docs/STAGE20_EYE_ROI_LABELING_PROTOCOL.md`.
Training requires a later, separate, explicit product-owner approval.

## Sponsor-demo status

The separate sponsor branch supplies a silent recorded-video engineering demo
for Windows x64 and Orin aarch64. It displays presence, monitoring availability,
eye metrics/events, PERCLOS, yawn, head counts, gaze/distraction availability,
drowsiness, and aggregate end-of-video statistics. It is not an accuracy or
safety-certification demonstration. Private meeting archives and videos remain
outside Git.

## Privacy boundary

Never commit or publish:

- personal recordings, audio, photographs, or identifying filenames;
- extracted eye/face frames or contact sheets;
- private subject/session mappings;
- raw annotations or per-frame traces; or
- binaries/archives that contain private recordings.

Private notes must use anonymous clip/subject/session IDs. Commit only approved
checksums, synthetic fixtures, protocols, tooling, and anonymous aggregates.
Do not upload private media to a third-party service without explicit consent.

## Useful collaborator tasks

1. Independently timestamp the selected dashcam inattention, yawn, obstruction,
   presence, and monitoring-unavailable intervals.
2. Review eye crops using the dense labeling protocol, keeping labels outside
   Git and model predictions hidden during first-pass review.
3. Repeat tests across visible light, IR, glasses, low light, distance, pose,
   partial face, and natural negative behavior.
4. Investigate gesture ideas as provider-neutral temporal FSMs, with start,
   confirmation, hold, release, cancellation, refractory, and unknown-quality
   behavior explicitly defined.
5. Report negative results and confounders, not just successful clips.

For hand/object gestures, keep hand geometry, object classification, and face
geometry separate. A hand near the mouth is not by itself a cigarette, drink,
or yawn event.

## Reproducible reporting template

```text
Objective:
Branch and exact commit:
Target / OS / architecture:
Release or Debug:
Backend and model checksum:
Anonymous private clip ID and checksum:
Duration / resolution / nominal FPS:
Lighting / eyewear / camera placement:
Exact command or procedure:
Ground-truth intervals and annotation status:
Observed events and intervals:
Precision / recall / F1 / count error / delay:
False alarms per hour and confounders:
Mean and p95 latency / FPS / CPU / memory:
Number of repeated runs:
Private artifacts retained and retention location:
Conclusion and next experiment:
```

Screenshots are supporting evidence only. Prefer timestamped annotations,
machine-readable traces, and repeated Release runs.

## Actions requiring product-owner coordination

Coordinate before:

- changing accepted thresholds, event definitions, or the frozen roadmap;
- training/fine-tuning a model or expanding the approved private-data use;
- choosing or rejecting the production model stack;
- adding runtime dependencies, accelerators, or model licenses;
- defining recognition enrollment/retention or object taxonomy;
- committing aggregates derived from private data;
- distributing private-media packages; or
- merging branches or publishing a release.

## Bootstrap prompt for a collaborator assistant

```text
Help me test the DMS project at https://github.com/ShrinivasAtre/face as an
independent collaborator. Read AGENTS.md, docs/DEVELOPMENT_PLAN.md,
docs/NEXT_DEVELOPMENT_HANDOFF.md, docs/FOLLOW_ON_DEVELOPMENT_PLAN.md, and
docs/COLLABORATOR_TESTING_HANDOFF.md completely before acting. Use the exact
branch and commit named in my task. Preserve the CMake/C++ provider-neutral
architecture. Keep personal recordings, audio, images, crops, mappings,
annotations, traces, and private result files outside Git. Do not train a model,
change accepted thresholds, merge, publish, or expand private-data use without
the product owner's explicit approval. Treat my results as experimental
evidence for review.
```
