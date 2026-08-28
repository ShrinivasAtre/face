# Sponsor Demo Validation Evidence

Validation date: 2026-08-27

Packaged source revision: `94ccdda2831297a11076f83d610dd87a79750b30`

## Privacy boundary

The packages contain no personal photographs, recordings, audio, extracted frames, or biometric annotations. The only included test input is `demo/synthetic_eye_sequence.csv`, a generated semantic sequence with expected FSM states.

## Windows x64

- Clean root: `D:\work\p20-sponsor-demo\build-windows`
- Release build: passed
- CTest: 12/12 passed
- Installed `run_self_test.ps1`: passed
- Package architecture/dependency/model/package-boundary checks: passed
- Archive: `face-dms-sponsor-demo-windows-x64.zip`
- Archive SHA-256: `c8d816ed8b869f084e49fa2b9a0aa23b79b664125a6ff6528b2624825e926567`

## NVIDIA Jetson Orin / Linux aarch64

- Clean root: `/home/shrinivas/common/p20-sponsor-demo/build-orin`
- Release build with GCC 13.3.0: passed
- CTest: 12/12 passed
- Installed `run_self_test.sh`: passed
- ARM aarch64 architecture, dependency, model, and package-boundary checks: passed
- Archive: `face-dms-sponsor-demo-orin-aarch64.tar.gz`
- Archive SHA-256: `88fff6d2d97d584917c93e6f3123e912429d13cb59a3a327442daa4fe8a5ced7`

## Scope of the validation

This gate proves clean native builds, deterministic temporal-eye behavior, package completeness, architecture, checksums, and launcher installation. It does not claim production DMS accuracy. The live camera preview still uses the legacy diagnostic EAR counter and requires a connected camera plus visual observation; its limitations are stated in `docs/SPONSOR_DEMO.md`.
