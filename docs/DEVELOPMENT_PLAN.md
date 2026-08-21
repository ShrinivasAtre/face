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
- Last completed step: **Step 7**
- Next formal step: **Step 8 — Introduce semantic eye landmarks**
- Step 7 implementation commits: `b66b3e12a776f2d7f2d6ee3e4eec53d3178dffc0`, `24a08950f7677158949d4fc6c6b415ecfd479be4`
- Bazel: `7.4.1`
- MediaPipe release: `v0.10.33`
- Pinned MediaPipe commit: `3987048d4b390aa9ae675c796f6421bbeece6511`
- Formal progress: **7 of 16 steps complete (43.75%)**

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
| 7 | Decouple blink tracker from LBF | COMPLETE | `LbfLandmarkDetector` owns model loading and fitting; `BlinkTracker` consumes supplied landmarks and reports processing success separately from eye state. Focused and real-image legacy integration tests pass on Windows x64 and Orin aarch64. |
| 8 | Introduce semantic eye landmarks | PARTIAL | A six-point semantic eye contract and LBF-specific mapper are implemented; `BlinkTracker` consumes only semantic eyes. Windows build and all focused/real-image tests pass. Orin validation remains. |
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

### Step 7 — Decouple blink tracker from LBF

- LBF model loading and `cv::face::Facemark::fit()` moved into `LbfLandmarkDetector`.
- `BlinkTracker` no longer includes OpenCV face headers, owns a Facemark object, loads a model, or fits landmarks.
- `BlinkTracker` consumes caller-supplied landmarks and returns processing success independently of `isEyeClosed()`.
- The existing YuNet/LBF application path now explicitly composes face detection, landmark acquisition, and blink processing.
- Focused tests validate open/closed EAR values, transition-only blink counting, repeated closed frames, invalid and non-finite landmarks, state clearing, and recovery.
- A reusable optional CTest exercises the real-image YuNet to LBF to `BlinkTracker` chain without requiring a camera or GUI.
- Windows x64 and Orin aarch64 CMake application builds passed with OpenCV 4.8.0.
- Both platforms detected one face, produced 68 LBF landmarks, and reported identical bbox `x=246 y=437 w=338 h=452`, right EAR `0.225832`, and left EAR `0.238513` on the established test image.
- Implementation commits: `b66b3e12a776f2d7f2d6ee3e4eec53d3178dffc0`, `24a08950f7677158949d4fc6c6b415ecfd479be4`.

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

Begin **Step 8 — Introduce semantic eye landmarks**.

Do not redesign Steps 1–7. Define a backend-neutral semantic representation for the six ordered points of each eye, then make blink processing consume that representation instead of raw 68-point topology indices.

### Step 8 objective

Introduce a backend-neutral eye-landmark contract that expresses the six points required by the existing EAR calculation without exposing LBF or MediaPipe topology indices to `BlinkTracker`.

For each subject eye, the ordered semantic points are:

1. outer corner;
2. upper outer lid;
3. upper inner lid;
4. inner corner;
5. lower inner lid;
6. lower outer lid.

The ordering preserves the current EAR pairs `(1,5)`, `(2,4)`, and horizontal pair `(0,3)` while giving each position an explicit meaning.

### Step 8 stages

1. Add a small common type representing the ordered right and left semantic eye points.
2. Move LBF indices `36–47` into an LBF-specific mapper that produces the common type.
3. Change `BlinkTracker` to accept only the semantic eye type.
4. Update the legacy application and integration test to compose LBF acquisition, LBF semantic mapping, and blink processing.
5. Add focused tests for semantic ordering, invalid source topology, non-finite coordinates, EAR equivalence, blink transitions, and recovery.
6. Build and run focused and real-image regression tests on Windows x64 and Orin aarch64.

### Step 8 acceptance gate

- `BlinkTracker` contains no raw facial-topology indices such as `36–47` and does not accept a full 68-point landmark vector.
- The common eye type has exactly six explicitly documented ordered points for each subject eye.
- LBF-specific indices exist only in the LBF semantic mapper.
- Existing EAR values, eye-state transitions, blink counts, and invalid-input recovery remain unchanged.
- The established real-image YuNet/LBF regression produces the same face box, 68 source landmarks, and EAR values on Windows and Orin.
- No MediaPipe indices or MediaPipe-specific headers/types are introduced; MediaPipe mapping remains Step 9.
- The existing CMake application and all focused tests pass on Windows x64 and Orin aarch64.
- Validation evidence and implementation commits are recorded before Step 8 is marked `COMPLETE`.

### Step 7 acceptance gate (completed)

- Move LBF model loading and `cv::face::Facemark::fit()` into a dedicated landmark-acquisition component.
- Remove `opencv2/face.hpp`, `cv::face::Facemark`, model loading, and landmark fitting from `BlinkTracker`.
- Make `BlinkTracker` consume caller-supplied landmarks while retaining the current EAR, drawing, calibration, eye-state, and blink-count behavior.
- Keep processing success distinct from the open/closed eye state.
- Preserve the existing YuNet/LBF application path by composing face detection, LBF landmark acquisition, and blink processing in `main.cpp`.
- Add focused tests for valid open/closed landmarks, blink transitions, repeated closed frames, invalid landmarks, and recovery.
- Do not introduce MediaPipe-specific types or perform the semantic-eye mapping reserved for Steps 8–9.
- Build and test the CMake application on Windows x64 and NVIDIA Orin/Linux aarch64, then record validation evidence before marking Step 7 complete.
