# Stage 19 PFLD Candidate Provenance

## Decision status

This document records a benchmark candidate, not a production-model approval.
No fetched repository or model binary is committed to this repository. The
candidate must pass the Stage 19 dataset and target-device gates before it can
replace LBF.

## Selected reproducible candidate

- Model: PFLD-class 68-point facial landmark detector
- Artifact name: `landmarks_68_pfld.onnx`
- Artifact size: 2,987,921 bytes
- SHA-256: `7d7bbd5c6a1d9272e58d9773898284a1905d872eba9a662df9b5f20f1ba6f83e`
- Topology: 68-point iBUG layout; model indices are permitted only inside the
  PFLD provider adapter.
- Input: one RGB face crop, NCHW float32, 112 x 112, normalized to `[0,1]`.
- Output: 136 float values interpreted as 68 normalized `(x,y)` pairs.
- Reproducible source: `mistial-dev/FaceOFFx`, commit
  `0e31288da5301a4ca73901e009a586f843595bcd`, path
  `src/FaceOFFx.Models/Resources/landmarks_68_pfld.onnx`.
- Upstream attribution recorded by that source: `FaceONNX/FaceONNX.Models`,
  inspected at commit `df11c0c73f46a9a675a16432374983fc5400d43a`.
- License claim: MIT in both cited repositories. Any redistributed package must
  retain the applicable MIT notices and this provenance record.

## Material limitations

- The exact training dataset, split, augmentation, training commit, checkpoint,
  export command, and published accuracy report for this binary are not
  documented by its upstream source. The license is traceable, but the training
  lineage is incomplete.
- Generic visible-light 68-point training cannot be assumed to generalize to
  near-IR illumination, sunglasses, darkness, large pose, partial face, or DMS
  camera placement. Those are mandatory slices in the target-data benchmark.
- Six contour points per eye may still be insufficient for calibrated eye
  opening and PERCLOS. Failure on those gates routes to Stage 20's dedicated eye
  ROI evaluation; it must not create PFLD-specific DMS logic.
- The crop transform is part of the model contract and must be benchmarked. The
  initial reference uses 10% face-box padding, aspect-preserving resize with gray
  padding, RGB channel order, and `[0,1]` scaling.

## Candidates not selected for initial integration

- `yakhyo/face-landmark-detection` commit
  `50a0b19fd96e89fe6c163d1e54dbd1ccabb0b899` contains a 98-point WFLW ONNX
  file (SHA-256
  `85d781e0c6484c6e07045998686651dc96a867957e8de4a9e8128ae6dd70ce34`),
  but no license file was present and no release or training-checkpoint lineage
  was documented. It is therefore not eligible for redistribution or the first
  controlled benchmark.
- `guoqiangqi/PFLD` and `polarisZhao/PFLD-pytorch` document WFLW training and
  PFLD implementations, but the inspected repositories do not provide a
  sufficiently explicit code-and-weight license/provenance chain for a
  production candidate.

## Required verification before use

1. Obtain the external artifact from the pinned source and reject any checksum
   mismatch.
2. Inspect ONNX input/output names, dtypes, shapes, and opset in the build
   evidence.
3. Keep crop/preprocessing and all 68-point indices inside the provider adapter.
4. Run the same downstream metric implementation for PFLD, LBF, and MediaPipe.
5. Record accuracy and resource evidence separately for Windows x64, x64
   Ubuntu, Orin aarch64, and later Raspberry Pi 5.
