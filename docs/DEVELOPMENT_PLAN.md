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
- Last completed step: **Step 10**
- Next formal step: **Step 11 — Pin/build MediaPipe dependency**
- Step 10 implementation commits: `b029605919e90ebe30fda7164c1327895552cb61`, `beb495a3323211ae76e741e9608179c7fc5f0a72`
- Bazel: `7.4.1`
- MediaPipe release: `v0.10.33`
- Pinned MediaPipe commit: `3987048d4b390aa9ae675c796f6421bbeece6511`
- Formal progress: **10 of 16 steps complete (63%)**

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
| 8 | Introduce semantic eye landmarks | COMPLETE | A six-point semantic eye contract and LBF-specific mapper isolate topology from `BlinkTracker`. Focused ordering/state tests and unchanged real-image regressions pass on Windows x64 and Orin aarch64. |
| 9 | MediaPipe eye-landmark mapping | COMPLETE | A dedicated mapper converts the documented twelve indices from the 478-point topology into semantic right/left eyes. Focused tests and unchanged legacy regressions passed on Windows x64 and Orin aarch64. |
| 10 | Self-contained DLL/SO | COMPLETE | Deterministic packaging scripts stage the bridge boundary and manifests. Clean packaged-runtime smoke tests passed on Windows x64 and Orin aarch64 with no external MediaPipe/TFLite/Abseil libraries. |
| 11 | Pin/build MediaPipe dependency | IN PROGRESS | Consolidate the full MediaPipe revision into one machine-readable source, verify Bazel/tag/commit/origin consistently, and validate idempotent fetch/build entry points on Windows x64 and Orin aarch64. |
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

### Step 8 — Introduce semantic eye landmarks

- Added a backend-neutral six-point semantic contract for each subject eye: outer corner, upper outer lid, upper inner lid, inner corner, lower inner lid, and lower outer lid.
- `BlinkTracker` now consumes only the semantic right/left eye representation and contains no raw facial-topology indices or full 68-point input API.
- LBF indices `36–47` are confined to `LbfEyeLandmarkMapper`.
- The LBF left eye is deliberately reordered from topology order `42–47` to semantic outer-to-inner order `45,44,43,42,47,46`; EAR pairings remain equivalent.
- Focused tests validate semantic EAR/state behavior, exact LBF ordering, short source topology, non-finite coordinates, result reset, blink transitions, and recovery.
- Windows x64 and Orin aarch64 CMake builds and all three CTests passed without requiring a live camera.
- The established real-image regression remained identical on both platforms: one face, 68 LBF landmarks, bbox `x=246 y=437 w=338 h=452`, right EAR `0.225832`, and left EAR `0.238513`.
- Orin application and test binaries were confirmed as ARM aarch64 with no unresolved dependencies.
- No MediaPipe topology or type was introduced; MediaPipe semantic mapping remains Step 9.
- Implementation commit: `2bfb63fa3abc96d8fa26aee91d1eba7e61083b90`.

### Step 9 — MediaPipe eye-landmark mapping

- Added `MediaPipeEyeLandmarkMapper`, which consumes backend-neutral pixel landmarks and produces the Step 8 semantic eye representation without exposing MediaPipe headers or C-ABI types.
- Confined the selected MediaPipe 478-point topology indices to the mapper and its focused test.
- Mapped the subject right eye as `33,160,158,133,153,144` and reordered the subject left eye into semantic outer-to-inner order as `263,387,385,362,380,373`.
- Enforced a minimum 478-point source topology and reset output on short input or non-finite selected x/y coordinates.
- Deliberately ignored z and non-selected coordinates because the shared EAR calculation uses only the twelve selected 2D points.
- Focused tests validate exact ordering, short topology, output reset, selected and non-selected non-finite values, optional non-finite z, and right/left EAR compatibility.
- Windows x64 built successfully and all four CTests passed; the new test executable was confirmed as x64.
- Orin built successfully at the exact implementation commit, all four CTests passed, and the relevant executables were confirmed ARM aarch64 with no unresolved application dependencies.
- The established YuNet/LBF real-image regression remained identical on both platforms: one face, 68 LBF landmarks, bbox `x=246 y=437 w=338 h=452`, right EAR `0.225832`, and left EAR `0.238513`.
- `BlinkTracker` and the legacy application path were not changed, and no live camera was required.
- Implementation commit: `94dc7ab02d0cf8f15ba732a841ff7622af2334d5`.

### Step 10 — Self-contained DLL/SO

- Added committed Windows and Linux packaging scripts with deterministic manifests, payload SHA-256 hashes, architecture checks, direct dependency inspection, exact C ABI export checks, and forbidden MediaPipe/TFLite/TensorFlow/Protobuf/Abseil runtime-dependency checks.
- Added optional packaged-runtime inputs to the existing smoke scripts so validation uses staged payloads rather than silently loading the Bazel output.
- The package boundary retains MediaPipe, TFLite, Abseil, and other Bazel implementation code inside the single bridge library.
- Windows packages `FaceMediaPipe.dll` and matching `opencv_world480.dll`; Windows system DLLs and the compatible Microsoft Visual C++ runtime remain documented platform prerequisites.
- Orin packages `libFaceMediaPipe.so`; OpenCV 4.8, EGL/GLES, the GNU C/C++ runtime, and the JetPack multimedia stack remain documented target-platform ABI prerequisites.
- Fresh Windows MediaPipe fetch/build passed at pinned commit `3987048d4b390aa9ae675c796f6421bbeece6511`; the build completed 4,530 actions in 3,929 seconds.
- Windows package reproduction produced identical manifest and payload hashes. Manifest SHA-256: `43b1a67a8d309e58a8e795c5eaa5a983817d6b7d728e6d2e0afcc7f848a36246`; bridge SHA-256: `7da6154bd7c51addf1a80fb9dfbd8122269c82030f90049ee56ef36cbfce89f4`.
- The packaged Windows runtime smoke test passed with one face, 478 landmarks, and bbox `x=243 y=491 w=350 h=410`.
- Fresh Orin MediaPipe fetch/build passed at the same pinned commit; the conservative build completed 3,217 actions in 3,752 seconds.
- Orin package reproduction passed byte-for-byte. Manifest SHA-256: `f57839c5deabb2919e393f6f449fa5339410d28a1fdefcec530d52d7ede204cf`; bridge SHA-256: `f216082eb9f8b36809e2531f3e4c8c0c4a28effb6515ea55a262bae2da18822e`.
- The packaged Orin runtime was confirmed ARM aarch64 with no unresolved dependency, and its smoke test passed with one face, 478 landmarks, and the same bbox.
- Both packages expose exactly the established five `face_mp_*` functions and contain no `.task` model; model deployment remains Step 14.
- Generated `dist/` output is ignored so re-packaging leaves both fresh validation repositories clean.
- No live camera was required.
- Implementation commits: `b029605919e90ebe30fda7164c1327895552cb61`, `beb495a3323211ae76e741e9608179c7fc5f0a72`.

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

Begin **Step 11 — Pin/build MediaPipe dependency**.

Do not redesign Steps 1–10. Consolidate the already-pinned dependency and platform build procedure into its formal Step 11 acceptance gate without replacing the application's CMake build system.

### Step 11 objective

Make the MediaPipe dependency selection and bridge build procedure mechanically reproducible from the repository. A single committed version file must define the release tag and full commit hash, and every platform fetch/build path must reject mismatched tools, origins, tags, commits, or checkouts before compiling.

### Step 11 stages

1. Replace duplicated short revision constants with a machine-readable version file containing the release tag and full 40-character commit.
2. Add cross-platform dependency-verification scripts for Bazel/Bazelisk, origin URL, tag resolution, exact checkout commit, and required MediaPipe workspace files.
3. Make both fetch scripts consume the same version file, fetch the named tag, detach at the full commit, and run verification.
4. Make both bridge build scripts run verification before applying compatibility patches or invoking Bazel.
5. Validate first fetch and repeated fetch behavior from fresh `p11` checkouts on Windows x64 and Orin aarch64.
6. Validate the committed build entry points and resulting bridge architecture/exports on both platforms, using the immediately preceding fresh Step 10 build evidence only where no build input changed.
7. Confirm the main CMake application remains independent of Bazel and MediaPipe headers.
8. Record validation evidence and commits before marking Step 11 complete.

### Step 11 acceptance gate

- `mediapipe/MEDIAPIPE_VERSION` is the only automation source of the MediaPipe tag and full commit hash used by fetch/build scripts.
- `.bazelversion` pins Bazel `7.4.1`, and verification rejects a different effective Bazel version.
- Windows and Linux verification reject an unexpected MediaPipe origin, tag target, checkout commit, missing workspace, or malformed version file.
- Both fetch scripts are idempotent and finish detached at exact commit `3987048d4b390aa9ae675c796f6421bbeece6511` for tag `v0.10.33`.
- Both build scripts verify the dependency contract before modifying the fetched workspace or invoking Bazel.
- The established Windows x64 DLL and Orin aarch64 SO build entry points pass and retain the five C ABI exports.
- Bazel remains limited to fetching/building the MediaPipe bridge; the application continues to use its existing CMake build.
- MediaPipe headers do not leak into the main application interface.
- No live camera or runtime model deployment is required for this dependency-build gate.
- Validation evidence and implementation commits are recorded before Step 11 is marked `COMPLETE`.

### Step 10 objective (completed)

Produce a reproducible deploy directory for the MediaPipe bridge on each target. MediaPipe, TFLite, Abseil, and other Bazel-built implementation dependencies must remain linked into the single bridge DLL/SO. Package non-system runtime files when practical, and explicitly document target-platform ABI prerequisites that must not be copied out of the operating system or JetPack installation.

The Step 10 package boundary is:

- Windows: `FaceMediaPipe.dll` plus the matching `opencv_world480.dll`; Windows system DLLs and the supported Microsoft Visual C++ runtime are platform prerequisites.
- Orin: `libFaceMediaPipe.so`; the installed OpenCV 4.8 ABI, EGL/GLES, C/C++ runtime, and JetPack multimedia stack are target-platform prerequisites.
- The runtime `face_landmarker.task` model is intentionally excluded until Step 14.

### Step 10 stages

1. Add Windows and Linux packaging scripts that fail clearly when the built bridge or required packaged runtime file is absent.
2. Generate a deterministic package manifest containing file names, SHA-256 hashes, architecture, direct shared-library dependencies, and the documented platform prerequisites.
3. Ensure Windows packages the matching OpenCV DLL beside the bridge and Orin records rather than copies its platform-owned shared-library ABI.
4. Confirm no MediaPipe, TFLite, TensorFlow, Protobuf, or Abseil shared library is required at runtime.
5. Run the existing smoke test from a clean deploy directory on Windows x64 and Orin aarch64 using the external model and still image.
6. Verify the five C ABI exports, architecture, dependency resolution, deterministic re-packaging, and repository cleanliness on both platforms.
7. Record validation evidence and commits before marking Step 10 complete.

### Step 10 acceptance gate (completed)

- Each committed packaging script produces a clean deploy directory using only explicit build outputs and documented platform inputs.
- Windows output contains `FaceMediaPipe.dll`, `opencv_world480.dll`, and a manifest; the DLL is x64 and all direct dependencies resolve from the package or supported Windows platform/runtime locations.
- Orin output contains `libFaceMediaPipe.so` and a manifest; the SO is ARM aarch64, has no unresolved dependency, and uses only documented OpenCV/JetPack/Linux platform libraries externally.
- Neither bridge has a runtime dependency whose name identifies MediaPipe, TFLite, TensorFlow, Protobuf, or Abseil.
- Both packaged bridges expose exactly the established five `face_mp_*` C ABI functions.
- Re-running packaging from the same build produces the same payload hashes and manifest.
- Existing still-image smoke tests pass from clean deploy directories with one face and 478 landmarks; no live camera is required.
- The `.task` model is not silently bundled before the Step 14 deployment policy is defined.
- Validation evidence and implementation commits are recorded before Step 10 is marked `COMPLETE`.

### Step 9 objective (completed)

Map the MediaPipe Face Landmarker 478-point output into the backend-neutral six-point semantic eye contract introduced in Step 8. Keep all MediaPipe topology indices inside a dedicated mapper and preserve the existing LBF path and `BlinkTracker` behavior.

The selected subject-eye mappings, expressed in semantic outer-to-inner order, are:

- right eye: `33, 160, 158, 133, 153, 144`;
- left eye: `263, 387, 385, 362, 380, 373`.

### Step 9 stages

1. Add a MediaPipe-specific mapper that accepts backend-neutral pixel landmarks and produces `SemanticEyeLandmarks`.
2. Confine all twelve MediaPipe topology indices to that mapper.
3. Add focused tests for exact semantic ordering, the required 478-point topology, non-finite selected coordinates, ignored non-selected coordinates, result reset, and EAR compatibility.
4. Build and run the CMake application and all focused tests on Windows x64 and Orin aarch64.
5. Run the established YuNet/LBF real-image regression to prove the legacy path is unchanged.
6. Record validation evidence and commits before marking Step 9 complete.

### Step 9 acceptance gate (completed)

- The MediaPipe mapper produces the established right/left semantic ordering using the documented twelve indices.
- MediaPipe topology indices exist only in the MediaPipe mapper and its focused test.
- The mapper rejects source results with fewer than 478 landmarks and rejects non-finite selected coordinates, resetting its output on every failure.
- Non-selected MediaPipe coordinates, including optional non-finite depth values, cannot corrupt the 2D eye mapping.
- `BlinkTracker` remains backend-neutral and unchanged.
- The existing YuNet/LBF application path and real-image regression remain unchanged.
- The CMake application and all focused tests pass on Windows x64 and Orin aarch64 without a live camera.
- Validation evidence and implementation commits are recorded before Step 9 is marked `COMPLETE`.

### Step 8 objective (completed)

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

### Step 8 acceptance gate (completed)

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
