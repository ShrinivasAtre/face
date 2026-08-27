# Stage 20 semantic integration — development results

Date: 2026-08-27

These are private development-set results, not production accuracy claims. Raw
recordings, audio, frames, annotations, and per-frame traces remain outside Git.
Only anonymous aggregate observations are recorded here.

## Implemented boundary

- Provider topology is confined to semantic adapters for mouth/pose points and,
  when available, iris gaze.
- Mouth openness is inner-lip separation normalized by mouth width.
- Head pose uses six semantic facial points, a generic 3D model, reprojection
  quality, and a stable one-second neutral calibration before FSM input.
- MediaPipe iris geometry supplies gaze. The 68-point LBF/PFLD providers report
  gaze unavailable instead of substituting head direction.
- Eye ROI quality measures visibility, exposure, contrast, and sharpness. Strong
  pose is low-quality for eye temporal metrics. Partial/out-of-frame eye regions
  are occluded; landmark presence alone is not an occlusion signal.
- Recorded benchmark schema 4 connects eye, yawn, pose, gaze/distraction,
  presence, and combined drowsiness FSMs and emits private per-frame evidence.

## Preliminary development observations

| Anonymous slice | Frames | Relevant aggregate result |
| --- | ---: | --- |
| Subject A visible yawn | 900 | 2 yaw events; presence `present`; final drowsiness `warning` from recent-yawn evidence |
| Subject A visible neutral | 900 | 0 pose counts; 0 distracted frames after neutral-pose calibration |
| Subject A visible head pose | 900 | 1 left and 1 right count; 208 distracted frames |
| Subject A visible occlusion | 900 | 870 detected frames; 132 eye-unknown frames; in-frame hand coverage was not classified as occluded |
| Subject B IR yawn | 900 | 4 yawn events; presence `present`; final drowsiness `warning` |
| Subject B IR head pose | 1,200 | 1 left, 1 right, 1 up, and 1 down count |

On the IR head-pose slice, pose-aware eye quality changed 398 strongly oblique
frames to low-quality and reduced apparent prolonged-closure frames from 165 to
zero while preserving all four directional counts.

The visible gaze/head slice had mean inter-eye gaze agreement 0.86. Horizontal
gaze ranged approximately -0.37 to +0.42, while the visible neutral slice ranged
approximately -0.04 to +0.04. Preliminary horizontal gaze enter/exit values were
therefore changed to 0.20/0.12. Vertical defaults remain more conservative.

## Open gates and limitations

- Event-time annotations are not yet available, so event precision/recall,
  detection delay, and exact count error cannot be claimed.
- The current image-quality observer cannot reliably distinguish an in-frame
  hand/object covering an eye from other textured eye-region content. A trained
  eye-ROI quality/occlusion candidate needs annotated normal, closed-eye,
  glasses, IR, hand/object occlusion, blur, and exposure samples. Until then,
  missing/partial/low-quality inputs are safely unknown, but the full occlusion
  acceptance gate remains open.
- Neutral calibration assumes a stable forward interval at session start. A
  production calibration/reacquisition policy requires product approval.
- Thresholds are preliminary engineering defaults. Stage 20 operational
  definitions and display/count/record/alert policies still require the planned
  user approval after broader annotated evaluation.
