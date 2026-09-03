# TI SK-AM62 platform enablement plan

Date: 2026-09-03

## Claim status

TI SK-AM62 support is planned, not implemented or validated. The
platform-independent C++17 semantic core is expected to transfer, but camera,
inference, acceleration, packaging, thermals and performance require native
device evidence.

## Initial assumptions to verify

- Exact board variant, RAM, storage and camera interface are not yet recorded.
- Exact Processor SDK Linux/BSP and kernel versions are not selected.
- CPU-only OpenCV is the bring-up baseline.
- Hardware acceleration is a separate candidate, not a prerequisite for first
  build success.
- MediaPipe availability/performance is not assumed.

## Work packages

| # | Work package | Autonomous preparation | Device-dependent gate |
|---:|---|---|---|
| A1 | Hardware/BSP inventory | Define machine-readable metadata fields | Board, SDK image, kernel, CPU, memory and power mode recorded |
| A2 | Toolchain and CMake | Audit C++17/POSIX assumptions; prepare native/cross-build matrix | Minimal dms_core build and CTests pass |
| A3 | OpenCV | Define required modules and version/build options | Native OpenCV build/import and real image tests pass |
| A4 | Camera | Reuse capture interface and define GStreamer/V4L2 adapter options | Intended camera format, resolution, FPS and timestamps validated |
| A5 | Providers | Retain YuNet/LBF reference and provider-neutral outputs | At least one viable detector/landmark path passes accuracy/resource gate |
| A6 | Acceleration | Define replaceable execution-provider boundary | Selected TI runtime conversion and parity evidence, if used |
| A7 | Packaging | Reuse manifest/hash contract and Linux launcher model | Native ELF/aarch64 dependencies and clean-device launch verified |
| A8 | Sustained validation | Reuse benchmark/result schema | Accuracy, CPU, memory, power, thermal and throttling run passes |

## Required native evidence

1. uname, distribution/BSP, kernel, compiler, CMake and OpenCV versions.
2. CPU topology, online cores, frequency governor, RAM and storage.
3. Camera node/pipeline, pixel format, resolution, nominal/measured FPS and
   monotonic timestamp behavior.
4. Release build of dms_core and all provider-independent tests.
5. Real-image provider tests and deterministic recorded-video benchmark.
6. p50/p95/p99 latency, throughput, per-core/overall CPU, RSS, power and
   temperature.
7. Thirty-minute minimum sustained run without unbounded latency/memory growth
   or thermal failure.
8. Deterministic package manifest, ELF architecture/dependency audit and
   clean-device launch.
9. Target-camera accuracy evidence before any DMS capability claim.

## Inputs needed later

- Physical SK-AM62 board and intended camera.
- Board/RAM/storage variant and intended production power envelope.
- Approved Processor SDK/BSP image.
- Whether TI acceleration is required or CPU baseline is acceptable.
- Network/SSH availability for autonomous device work.

No credentials should be placed in this repository or document.

