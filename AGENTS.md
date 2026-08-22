# Face Project Agent Instructions

These instructions apply to the entire repository.

## Authoritative context

- Read `docs/DEVELOPMENT_PLAN.md` completely before planning follow-on work. It is the authoritative record of the completed 16-step MediaPipe integration program.
- Read `docs/NEXT_DEVELOPMENT_HANDOFF.md` for the current repository checkpoint, platform paths, toolchain, and operating procedure.
- Do not redesign, repeat, or reopen the completed 16 steps unless a verified regression requires it.
- Treat `main` at the checkpoint recorded in the handoff as the accepted baseline.

## Working method

- Before changing code, verify the current branch, `HEAD`, remote tracking state, and working-tree cleanliness.
- Define each new formal stage's objective, scope, acceptance criteria, and required Windows/Orin evidence in a tracked follow-on roadmap before implementation.
- Use a fresh numbered validation root for each formal stage: `D:\work\pN` on Windows and `~/common/pN` on Orin.
- Preserve existing user changes and unrelated files. Never discard or overwrite them to make a checkout clean.
- Keep CMake as the application build system. Bazel remains limited to the MediaPipe bridge/dependency boundary.
- Keep MediaPipe headers and implementation dependencies out of the main application interface and preserve runtime bridge loading.

## Autonomy and user interaction

- For an implementation request, autonomously inspect, edit, build, test, package, and run non-destructive diagnostics on both Windows and Orin when they are within the stated scope.
- Do not ask the user to copy commands between machines when the same action can be performed through the local shell or the configured Orin SSH connection.
- Batch safe, related validation commands and keep the user informed during long builds.
- Ask the user only for an unavoidable physical or secret-bearing action, a destructive operation, an external scope expansion, or a product decision that materially changes the result.
- Never request, print, copy, or commit a private key, passphrase, token, or password.
- Notify the user before any test needs a live camera. A single camera may need to be physically moved between Windows and Orin.
- Live GUI behavior requires user observation unless an explicitly authorized remote GUI/control mechanism is available. Prefer still-image and headless tests first.

## Windows and Orin validation

- Windows target: x64. Orin target: NVIDIA Jetson Orin/Linux aarch64.
- Validate focused tests first, then full platform builds, still-image integration, deterministic packaging, dependency/architecture checks, and live camera only when required by the acceptance gate.
- Require the same backend contract on both platforms: `--backend=yunet` and `--backend=mediapipe`.
- Confirm the main executable does not directly link against `FaceMediaPipe.dll` or `libFaceMediaPipe.so`.
- Confirm the pinned task model and dependency revisions against their committed checksum/version contracts.
- Do not call a cross-platform stage complete until its required Windows and Orin evidence passes.

## Git and documentation

- Base follow-on work on current `main` and use a purpose-specific feature branch unless the user explicitly directs otherwise.
- Stage only files belonging to the current task. Review diffs and run `git diff --check` before committing.
- Record implementation commits, validation commands, meaningful results, and remaining limitations in the follow-on roadmap.
- Pushing the current follow-on feature branch is permitted when requested work is ready and authentication is already available. Ask before merging or directly advancing `main` unless the user has explicitly authorized that integration.
- Keep generated builds, fetched dependencies, deployment directories, credentials, and test inputs out of Git unless the project deliberately tracks them.
