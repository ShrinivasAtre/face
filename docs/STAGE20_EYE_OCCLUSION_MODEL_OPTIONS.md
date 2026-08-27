# Stage 20 trained eye-ROI occlusion options

Date: 2026-08-27

## Decision being evaluated

The existing image-quality heuristic detects missing, partial/out-of-frame,
blurred, and poorly exposed eye observations, but cannot reliably distinguish a
textured hand/object covering an in-frame eye from a visible eye. Landmark
presence is not an occlusion signal. This evaluation considers a model that
produces provider-neutral semantic eye quality and can run from native C++ on
Windows x64, Ubuntu x64, Jetson Orin, and Raspberry Pi 5.

## Options

| Option | Strength | Limitation | Decision |
| --- | --- | --- | --- |
| Continue heuristic only | No new model/runtime cost | Cannot close the observed in-frame occlusion gate | Reject as production solution; retain as fallback/pre-filter |
| Binary visible/occluded ROI classifier | Small and easy to schedule | Confuses closed eye, glasses, blur, exposure, and partial crop; poor diagnostic value | Do not use as the final label contract |
| Multi-class lightweight eye-ROI classifier | Directly separates visible-open, visible-closed, partial, hand/object occluded, glasses, blur/exposure/invalid | Requires representative annotations and careful class balance | Recommended first trained baseline |
| Multi-task classifier plus openness regression | One scheduled inference supplies visibility/occlusion and provider-neutral openness | More annotation and loss-balancing work | Recommended controlled comparison against classifier-only baseline |
| Eye/occluder segmentation | Best spatial explanation and potentially strongest partial-occlusion handling | More labels, compute, and conversion risk on Raspberry Pi | Escalation option only if classification misses the gate |
| Specialized eye landmarks only | Improves eye geometry/openness | Still does not independently prove hand/object occlusion | Useful eye baseline, not the occlusion solution |

## Recommended baseline contract

- Input: aligned left/right eye crops, fixed shape initially benchmarked at
  96x64 or 128x80, with side normalization and no identity output.
- Backbone: MobileNetV3-Small-class CNN as the conservative starting point.
  MobileNetV3 was designed using hardware-aware search for mobile CPUs, making
  it a more appropriate first baseline than a large general classifier.
- Outputs per eye: `visible_open`, `visible_closed`, `partial`,
  `occluded_hand_or_object`, `glasses_degraded`, `invalid_quality`, plus optional
  continuous openness and calibrated confidence.
- Fusion: both eyes are evaluated independently; DMS logic receives only
  semantic states/confidences. It never receives model topology or class-index
  assumptions directly.
- Scheduling: do not run every frame. Run at a base cadence, increase cadence
  during uncertain/closing transitions, cache only while fresh, and force
  reacquisition after face loss, strong pose, or stale ROI geometry.

## Runtime architecture

Export one fixed-shape ONNX model and keep inference behind a C++ abstraction.
ONNX Runtime exposes a common execution-provider API across C/C++ targets and
supports CPU, CUDA, TensorRT, DirectML, and edge providers. On Orin, register
TensorRT first, CUDA second, and CPU last; ONNX Runtime explicitly recommends a
CUDA fallback for nodes TensorRT cannot execute. On Windows, evaluate DirectML
and CPU, with CUDA where the deployment GPU supports it. Ubuntu and Raspberry
Pi begin with CPU; Hailo remains a later device-specific conversion path rather
than changing the semantic model contract.

References:

- [ONNX Runtime execution providers](https://onnxruntime.ai/docs/execution-providers/)
- [ONNX Runtime TensorRT provider and C++ configuration](https://onnxruntime.ai/docs/execution-providers/TensorRT-ExecutionProvider.html)
- [ONNX Runtime DirectML provider](https://onnxruntime.ai/docs/execution-providers/DirectML-ExecutionProvider.html)
- [NVIDIA TensorRT C++/ONNX API](https://docs.nvidia.com/deeplearning/tensorrt/latest/api/c-api.html)
- [MobileNetV3 paper](https://arxiv.org/abs/1905.02244)

## Data and acceptance experiment

Use subject/session-disjoint development and final-test splits. Required labels
are normal open/closed, ordinary and long blink, partial eye, hand/object
occlusion, clear/dark glasses, visible/IR, low light, blur, over/under-exposure,
distance, and head pose. Hard negatives must include fingers near but not over
the eye, eyewear reflections, eyebrows/hair, and genuine closed eyes.

Compare classifier-only and multi-task variants using:

- per-class precision/recall/F1 and confusion matrix;
- occlusion false alarms per hour and missed-occlusion duration;
- eye-openness error, blink count error, PERCLOS error, and detection delay;
- known-eye coverage under pose/IR/glasses;
- p50/p95 latency, FPS, CPU/GPU utilization, memory, package size, and sustained
  thermals on Windows, Orin, Ubuntu, and later Raspberry Pi;
- FP32 versus FP16 on Orin and FP32 versus INT8 only after accuracy calibration.

## Current recommendation

Proceed to a controlled **MobileNetV3-Small-class multi-class eye-ROI baseline**
and compare it with the same backbone plus openness regression. Do not begin
training or select a production model until Task 4 labels cover the required
classes and the user approves the training/data policy gate. Do not adopt an
unverified downloadable eye-state model merely because it exports to ONNX;
license, provenance, IR performance, occlusion taxonomy, and subject-disjoint
accuracy must be established first.

Task 5 option evaluation is complete at architecture level. Dataset creation,
training, and device benchmarking are follow-on implementation work, not yet an
accuracy result.
