# Stage 21 pretrained candidate register

Date: 2026-09-03

## Decision

Benchmark only candidates that can run offline behind the C++ provider boundary.
Do not select or distribute a production model until both the weight license and
training-data provenance are acceptable for the intended product.

| Candidate | Purpose | Technical fit | Rights/provenance result | Decision |
|---|---|---|---|---|
| OpenCV Zoo SFace `2021dec` ONNX | Recognition | Native `cv::FaceRecognizerSF`; CPU/OpenCV path; compact | Model directory contains Apache-2.0, but exact training-data/weight commercial clarification is unresolved publicly | Evaluation baseline only |
| Open Model Zoo ResNet100 ArcFace ONNX | Recognition | Higher-cost ONNX CPU reference | Public conversion path; exact weight/training-data product rights require further review | Optional evaluation reference; do not package |
| InsightFace `buffalo_*` public packs | Recognition | Technically capable ONNX packs | Official model zoo limits public weights to non-commercial research | Excluded from this product baseline |
| Open Model Zoo `anti-spoof-mn3` | PAD | 128x128 MobileNetV3; 0.15 GFLOPs; CPU support | Official card states original model MIT and CelebA-Spoof training | Evaluation candidate; camera/domain validation mandatory |
| Silent-Face-Anti-Spoofing MiniFASNet | PAD | Lightweight; available model architecture | Apache-2.0 repository, but third-party conversion and camera sensitivity add provenance risk | Secondary research candidate only |

## Primary sources reviewed

- SFace directory license: https://github.com/opencv/opencv_zoo/blob/main/models/face_recognition_sface/LICENSE
- SFace C++ reference: https://github.com/opencv/opencv_zoo/blob/main/models/face_recognition_sface/demo.cpp
- SFace provenance clarification remains open: https://github.com/opencv/opencv_zoo/issues/313
- InsightFace model restriction: https://github.com/deepinsight/insightface/blob/master/model_zoo/README.md
- OMZ anti-spoof model card: https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/public/anti-spoof-mn3/README.md
- OMZ device support: https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/public/device_support.md

## Benchmark contract

The first baseline uses SFace only as an evaluation artifact. Acquisition must
record URL, byte size, SHA-256, acquisition date, directory license, runtime,
preprocessing, test-fixture provenance, CPU/platform, warmup count, repetitions,
latency distribution, genuine scores, impostor scores, and errors. No score from
that fixture becomes a product threshold.

PAD is reported separately. Passing recognition never compensates for a spoof or
indeterminate PAD result.
