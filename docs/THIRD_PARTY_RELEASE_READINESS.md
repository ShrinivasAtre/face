# Third-party and model release readiness

Date: 2026-09-03

This is an engineering compliance record, not legal advice. It separates
reproducible identity from redistribution permission: a version or SHA-256
identifies bytes, but does not establish rights to ship them.

## Current release blockers

| Item | Identity evidence | Rights/provenance status | Required closure |
|---|---|---|---|
| DMS Next source | Git revision recorded in packages | Repository has no declared product license | Product owner selects and commits license/copyright statement |
| OpenCV/contrib | Validated 4.8.0 baseline; package hashes generated | Apache-2.0; exact build may contain separately licensed third-party code | Preserve license/NOTICE and generate notices from exact build configuration |
| YuNet ONNX | OpenCV Zoo commit 47534e27c9851bb1128ccc0102f1145e27f23f98; its Git LFS object SHA-256 exactly matches ebafce4e3c118d6554634be5c27ab333b4c047a9a8c3faf1d7cf93101c22f0f0 | OpenCV Zoo model directory records MIT; upstream states this is a dynamic-shape re-export of the 2023mar model | Retain the MIT model notice and this exact source/hash record; confirm training-data/product suitability before release |
| LBF YAML | SHA-256 70dd8b1657c42d1595d6bd13d97d932877b3bed54a95d3c4733a0f740d1fd66b | Exact source revision, model terms and training lineage are not recorded | Reconstruct provenance and required attribution for exact bytes |
| MediaPipe source | v0.10.33; commit 3987048d4b390aa9ae675c796f6421bbeece6511 | Apache-2.0; transitive graph has additional terms | Generate dependency inventory/notices from the exact bridge build |
| Face Landmarker task | SHA-256 64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff | Exact model download record, model card and weight terms are not committed | Record authoritative artifact URL, model card, terms and limitations |
| PFLD evaluation model | External SHA-256 7d7bbd5c6a1d9272e58d9773898284a1905d872eba9a662df9b5f20f1ba6f83e | MIT claim recorded; training lineage incomplete | Keep evaluation-only unless provenance and product gate are accepted |
| SFace evaluation model | External, acquisition record in Stage 21 evidence | Directory license exists; weight/training clarification remains open | Keep evaluation-only; do not package as production model |
| PAD evaluation model | External Open Model Zoo candidate | Model card records origin/license/training dataset | Confirm exact downloaded artifact hash and all conversion/runtime terms |

## Deterministic source SBOM

The generate_source_sbom.ps1 script produces a CycloneDX 1.5 JSON inventory
from the repository's committed contracts and model bytes. It:

1. rejects missing or checksum-mismatched committed models;
2. records the Git revision and dirty state;
3. records exact MediaPipe and Bazel pins;
4. distinguishes installed/build-resolved OpenCV from source-pinned components;
5. marks unresolved licenses/provenance explicitly; and
6. emits no private paths, credentials, recordings or biometric data.

Example:

    .\scripts\generate_source_sbom.ps1 -OutputPath .\dist\compliance\dms-next-source.cdx.json

This source SBOM is not the final distribution SBOM. Before release, run a
binary/package scanner on each actual Windows, Ubuntu, Orin, Raspberry Pi or
AM62 payload and reconcile every DLL/SO, model, codec and runtime with the
source inventory.

## Release evidence checklist

1. Project license and copyright ownership approved.
2. Exact source, revision, hash, license and model-card record for every model.
3. Compiler/runtime, OpenCV build options, OS/BSP and accelerator versions.
4. Direct and transitive DLL/SO inventory from the final package.
5. Consolidated third-party notices and corresponding license texts.
6. Cryptographic package manifest verified after packaging.
7. Legal/product sign-off for intended jurisdictions and distribution channel.

## Primary provenance sources

- YuNet exact introduction commit and Git LFS identity:
  https://github.com/opencv/opencv_zoo/commit/47534e27c9851bb1128ccc0102f1145e27f23f98
- YuNet model documentation:
  https://github.com/opencv/opencv_zoo/tree/47534e27c9851bb1128ccc0102f1145e27f23f98/models/face_detection_yunet
- LBF candidate source location:
  https://github.com/kurnianggoro/GSOC2017/blob/master/data/lbfmodel.yaml
- MediaPipe Face Landmarker published model URL:
  https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task

The LBF location and MediaPipe download URL improve traceability, but they do
not close the corresponding weight-license and exact-byte release approvals.
