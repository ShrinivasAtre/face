# Sponsor video catalog

The external demonstration ZIP contains a curated set of consented development
recordings and representative dashcam clips. The media itself is private and is
not stored in Git. Confirm permission to show the selected clip before a demo.

| Video | Intended demonstration |
|---|---|
| `01-visible-neutral.mp4` | Baseline presence, calibration and neutral state |
| `02-visible-blinks.mp4` | Ordinary/long blink behavior |
| `03-visible-eye-closure.mp4` | Long blink and prolonged closure |
| `04-visible-yawn.mp4` | Mouth openness and yawn FSM |
| `05-visible-head-pose.mp4` | Head zones and directional counts |
| `06-visible-gaze-head.mp4` | Gaze versus head movement and distraction |
| `07-visible-occlusion.mp4` | Quality loss and monitoring availability |
| `08-ir-neutral.mp4` | Near-IR baseline |
| `09-ir-blinks.mp4` | Near-IR eye-event behavior |
| `10-ir-eye-closure-glasses.mp4` | Near-IR closure, gaze and clear glasses |
| `11-ir-head-pose.mp4` | Near-IR head movement |
| `12-ir-yawn.mp4` | Near-IR yawn behavior |
| `13-dashcam-inattention.mp4` | Small-face inattention sample |
| `14-dashcam-yawn.mp4` | Small-face yawn sample |
| `15-dashcam-camera-obstruction.mp4` | Absence/monitoring-unavailable behavior |

The filename describes the intended content, not verified ground truth. The
displayed event counts remain development outputs until timestamped annotations
pass the Stage 20 accuracy gate. In particular, dashcam labels originate from
their source folders and must be independently timestamp-verified before being
used as accuracy evidence.
