# Face Project — Next Development Handoff

## Accepted checkpoint

- Repository: `ShrinivasAtre/face`
- Canonical branch: `main`
- Validated revision: `755d9ffc8dcaaa170f29b2082082d5d4e6f1b4d9`
- Formal program status: **16 of 16 steps complete (100%)**
- Full history and acceptance evidence: `docs/DEVELOPMENT_PLAN.md`

At this revision, `main` and `feature/mediapipe-step1-backend-interface` point to the same commit. Windows x64 and NVIDIA Jetson Orin/Linux aarch64 passed enabled builds, all eight registered tests, deterministic final packaging, dependency and architecture inspection, and live-camera runs with both YuNet/LBF and MediaPipe.

## Architecture that must remain intact

- The application continues to build with CMake.
- YuNet/LBF remains the default backend and MediaPipe remains optional.
- Runtime selection is strict: `--backend=yunet` or `--backend=mediapipe`.
- Both backends implement the backend-neutral `FaceBackend`/`FaceResult` contract and feed the common semantic-eye and blink-processing path.
- The main executable does not link directly to the MediaPipe bridge.
- MediaPipe is isolated behind the versioned five-function C ABI and runtime-loaded `FaceMediaPipe.dll`/`libFaceMediaPipe.so`.
- MediaPipe SDK headers do not enter the application interface.
- The task model is an external runtime asset at `models/mediapipe/face_landmarker.task`.
- MediaPipe is pinned to release `v0.10.33`, commit `3987048d4b390aa9ae675c796f6421bbeece6511`; Bazel is pinned to `7.4.1`.
- The task model SHA-256 is `64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff`.

## Machine and validation context

### Windows x64

- Recommended next-stage root: `D:\work\p17`
- Completed Step 16 checkout: `D:\work\p16\face-step16`
- Completed application package: `D:\work\p16\package-windows-a`
- Accepted MediaPipe runtime package input: `D:\work\p10\face-step10\dist\mediapipe\windows-x64`
- OpenCV CMake package: `C:\opencv-4.8.0-src\install\lib`
- OpenCV runtime DLL: `C:\opencv-4.8.0-src\install\bin\opencv_world480.dll`
- Established still image: `D:\work\p4\face-step4-clean-win\TestData\IMG-20150331-WA0001.jpg`

### NVIDIA Jetson Orin / Linux aarch64

- SSH endpoint: `shrinivas@192.168.1.41:22`
- Recommended next-stage root: `~/common/p17`
- Completed Step 16 checkout: `~/common/p16/face-step16`
- Completed application package: `~/common/p16/package-orin-a`
- Accepted MediaPipe runtime package input: `~/common/p10/face-step10/dist/mediapipe/linux-aarch64`
- Established still image: `~/common/p4/step4-validation-inputs/IMG-20150331-WA0001.jpg`
- Use the already authorized dedicated Windows-to-Orin SSH key. Its private material and path are host configuration, not repository data.

## Final Step 16 package launch commands

Windows PowerShell:

```powershell
Set-Location D:\work\p16\package-windows-a
.\run_face.ps1 yunet
.\run_face.ps1 mediapipe
```

Orin desktop terminal:

```bash
cd ~/common/p16/package-orin-a
./run_face.sh yunet
./run_face.sh mediapipe
```

## Procedure for follow-on development

1. Fetch `main` and verify the accepted checkpoint or a documented successor.
2. Create a purpose-specific feature branch from `main`.
3. Create `docs/FOLLOW_ON_DEVELOPMENT_PLAN.md` (or update an existing successor roadmap) with the new objective and acceptance criteria.
4. Use fresh `pN` checkouts/directories on Windows and Orin for every formal cross-platform stage.
5. Let the agent run all non-interactive Windows and SSH-accessible Orin commands directly.
6. Ask the user only when camera movement/visual confirmation, authentication, a destructive action, or a material product choice is unavoidable.
7. Commit evidence with the implementation, keep the feature branch synchronized, and merge to `main` only after its acceptance gate passes.

## New-chat bootstrap prompt

```text
Continue ShrinivasAtre/face from the completed MediaPipe integration.
Work from current main. First read AGENTS.md, docs/DEVELOPMENT_PLAN.md,
and docs/NEXT_DEVELOPMENT_HANDOFF.md completely. Treat them as the
authoritative history and operating instructions. Do not repeat the completed
16-step program. Verify the repository checkpoint, summarize the current
architecture, and define the next objective and acceptance gate before making
changes. Work autonomously on Windows and Orin; involve me only for unavoidable
physical, secret-bearing, destructive, external-scope, or material product
decisions.
```
