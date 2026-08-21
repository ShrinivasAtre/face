# Face Project Development Plan

This file is the authoritative progress ledger for the 16-step MediaPipe integration program.

Update it whenever a step changes state. Do not mark a step complete until its acceptance criteria have been validated and the supporting changes have been committed. Later-step work that happens early must be recorded as partial, not complete.

## Status legend

- `COMPLETE`: implementation and acceptance validation are finished.
- `PARTIAL`: useful implementation exists, but the step's formal acceptance gate has not been completed.
- `NOT STARTED`: no accepted implementation work has been completed for the step.
- `BLOCKED`: progress cannot continue until the documented blocker is resolved.

## Current checkpoint

- Repository: `ShrinivasAtre/face`
- Branch: `feature/mediapipe-step1-backend-interface`
- Last completed step: **Step 6**
- Next formal step: **Step 7 — Decouple blink tracker from LBF**
- Step 6 implementation commits: `d1617b1cba10c714290ee912a170b58bff9bdd2e`, `86b84e99f7af5c1c41d2617f8817d8f008e88028`
- Bazel: `7.4.1`
- MediaPipe release: `v0.10.33`
- Pinned MediaPipe commit: `3987048d4b390aa9ae675c796f6421bbeece6511`
- Formal progress: **6 of 16 steps complete (37.5%)**

## Baseline requirements

These requirements are non-negotiable throughout the program.

| Requirement | Target |
| --- | --- |
| Existing application | Continues to build and run |
| Application build system | Existing CMake build remains in place |
| Existing YuNet/LBF backend | Continues working unchanged initially |
| MediaPipe | Added as an optional backend |
| Runtime selection | `--backend=yunet` and `--backend=mediapipe` |
| Windows bridge | `FaceMediaPipe.dll` |
| NVIDIA Orin bridge | `libFaceMediaPipe.so` |
| MediaPipe dependencies | Self-contained in the DLL/SO as far as technically possible |
| Main executable | Does not link directly against MediaPipe |
| MediaPipe headers | Do not leak into the main application |
| Blink detection | Uses a common backend-independent processing layer |
| MediaPipe model | Runtime `.task` model |
| Cross-platform API | Same C ABI on Windows and Linux |
| Reproducibility | Pinned MediaPipe revision |
| Existing build mechanism | No wholesale replacement of CMake with Bazel |

## Program status

| # | Step | Status | Evidence and remaining work |
| ---: | --- | --- | --- |
| 1 | Introduce backend-independent interface | COMPLETE | `FaceBackend`, `FaceResult`, and backend-neutral landmark types exist. Backend-specific MediaPipe types do not appear in the application interface. |
| 2 | Preserve existing YuNet/LBF implementation | COMPLETE | The application still uses the existing YuNet detector and LBF `BlinkTracker`; the legacy path was deliberately preserved. |
| 3 | Add MediaPipe C ABI | COMPLETE | Versioned C ABI with an opaque handle, caller-owned result storage, error reporting, and five validated exports. |
| 4 | Implement MediaPipe backend | COMPLETE | Fresh-clone Windows x64 and Orin aarch64 builds and smoke tests passed. Linux script executable modes were corrected and committed in `09314d2`. |
| 5 | BGR to MediaPipe image conversion | COMPLETE | Private conversion helper and focused tests validate channel order, compact rows, padded strides, invalid inputs, and overflow. Bridge rebuilds and real-image regressions passed on Windows x64 and Orin aarch64. |
| 6 | Extract/normalize MediaPipe landmarks | COMPLETE | A private, tested conversion layer converts normalized x/y to image pixels, preserves z, enforces caller capacity, computes a clamped bounding box from all source points, resets empty results, and rejects invalid/non-finite input. Focused tests, bridge builds, and real-image regressions passed on Windows x64 and Orin aarch64. |
| 7 | Decouple blink tracker from LBF | NOT STARTED | `BlinkTracker` still owns `cv::face::Facemark`, loads the LBF model, and obtains landmarks internally. |
| 8 | Introduce semantic eye landmarks | NOT STARTED | No backend-neutral semantic left/right eye representation exists yet. |
| 9 | MediaPipe eye-landmark mapping | NOT STARTED | No accepted mapping from MediaPipe's 478-point topology to semantic eye landmarks exists. |
| 10 | Self-contained DLL/SO | PARTIAL | The MediaPipe implementation is isolated in a shared library, but Windows depends on `opencv_world480.dll` and Orin uses system shared libraries. Packaging policy and validation remain. |
| 11 | Pin/build MediaPipe dependency | PARTIAL | MediaPipe is pinned and clean fetch/build validation passed on both platforms. Retain this as a formal later gate until its complete acceptance criteria are recorded and passed. |
| 12 | Runtime DLL/SO loading | NOT STARTED | The main application has no accepted `LoadLibrary`/`dlopen` abstraction and does not consume the C ABI. |
| 13 | CMake integration | NOT STARTED | CMake builds the existing YuNet/LBF application but does not yet configure or stage the MediaPipe bridge. |
| 14 | Model deployment | NOT STARTED | The bridge accepts an external `.task` path, but the model is not deployed by the application build. |
| 15 | Runtime backend selection | NOT STARTED | The application has no accepted `--backend=yunet` / `--backend=mediapipe` option. |
| 16 | Final Windows/Orin deployment | NOT STARTED | Final application-level packaging and end-to-end deployment validation remain. |

## Completed-step evidence

### Steps 1–3

- Backend-neutral interface and result types were introduced without changing the active YuNet/LBF application path.
- The MediaPipe boundary uses a C ABI and opaque `FaceMPHandle`.
- Validated exported symbols:
  - `face_mp_api_version`
  - `face_mp_create`
  - `face_mp_destroy`
  - `face_mp_last_error`
  - `face_mp_process_bgr`

### Step 4 — MediaPipe backend

Step 4 was accepted only after reproducible fresh-clone validation on both target platforms.

#### Windows x64

- Fresh checkout at the validated Step 4 source revision.
- Bazel `7.4.1`.
- MediaPipe freshly fetched at exact commit `3987048d4b390aa9ae675c796f6421bbeece6511`.
- `FaceMediaPipe.dll` built successfully and was confirmed x64.
- All five C ABI exports were present.
- Runtime dependency inspection included `opencv_world480.dll`.
- Committed smoke-test script executed successfully.

#### NVIDIA Jetson Orin / Linux aarch64

- Fresh checkout at the validated Step 4 source revision.
- Architecture confirmed as aarch64.
- Bazel `7.4.1`.
- MediaPipe freshly fetched at the exact pinned commit.
- `libFaceMediaPipe.so` built successfully and was confirmed as an ARM aarch64 ELF shared object.
- `ldd` reported no unresolved dependencies.
- All five C ABI exports were present.
- Committed smoke-test script executed successfully.
- Linux scripts were changed from mode `100644` to `100755`, direct execution was validated, and the fix was committed.

#### Cross-platform smoke-test result

Both platforms used `IMG-20150331-WA0001.jpg`:

- Image size: `960x1280`
- Detected: `1`
- Landmarks: `478`
- Bounding box: `x=243 y=491 w=350 h=410`
- Result: `MediaPipe smoke test PASSED`

### Step 5 — BGR to MediaPipe image conversion

- Public C ABI remained unchanged.
- Production conversion was isolated in a private helper used by focused tests.
- Channel order, compact rows, independent padded strides, invalid inputs, and overflowing row sizes were validated.
- Focused Bazel tests and bridge rebuilds passed on Windows x64 and Orin aarch64.
- The established real-image regression passed on both platforms with one face, 478 landmarks, and the unchanged bounding box.
- Implementation commit: `9f6f15dec59aced11da9656f9c1a0e238c288177`.

### Step 6 — Extract/normalize MediaPipe landmarks

- Public C ABI remained unchanged.
- Production landmark conversion was isolated in a private helper used by focused tests.
- Normalized x/y coordinates are converted to image pixels; z is preserved unchanged.
- Caller capacity truncates the number of written landmarks without changing the bounding-box source set.
- The face bounding box is derived from all source landmarks and clamped to image bounds; landmark coordinates themselves remain unclamped.
- Empty results are reset, and invalid dimensions, pointers, counts, and non-finite coordinates are rejected.
- Fresh Step 6 bridge builds and the focused `landmark_conversion_test` passed on Windows x64 and Orin aarch64.
- The established real-image regression passed on both platforms with one face, 478 landmarks, and bounding box `x=243 y=491 w=350 h=410`.
- Implementation commits: `d1617b1cba10c714290ee912a170b58bff9bdd2e`, `86b84e99f7af5c1c41d2617f8817d8f008e88028`.

## Historical YuNet/LBF observations

The repository contains earlier manual YuNet/LBF robustness results. They record blink over-counting, sensitivity to distance and lighting, and imperfect eye-landmark placement. These are historical observations about the legacy implementation and must not be misclassified as failures of the new MediaPipe bridge.

The legacy backend must remain available until the program deliberately replaces or refactors shared behavior in later steps.

## Progress update procedure

For every step:

1. Record the step-specific objective and acceptance criteria before implementation.
2. Preserve all baseline requirements.
3. Implement only the current step unless a dependency requires narrowly scoped early work.
4. Validate on the platforms required by that step.
5. Record commands, meaningful results, and relevant commit IDs.
6. Mark the step `COMPLETE` only after its acceptance gate passes.
7. Update the current checkpoint and formal completion count.
8. Commit the plan update with the implementation or immediately after validation.

## Next checkpoint

Begin **Step 7 — Decouple blink tracker from LBF**.

Do not redesign Steps 1–6. Start by documenting the current `BlinkTracker` responsibilities and defining a backend-independent boundary that allows externally supplied landmarks while preserving the existing YuNet/LBF behavior.
