# Sponsor feedback implementation tracker

Date: 2026-09-03

This tracker converts the sponsor feedback into numbered, independently
reviewable work packages. Status describes repository evidence, not a released
or safety-certified product.

| # | Work package | Status | Autonomous next action | External decision or resource |
|---:|---|---|---|---|
| 1 | Coordinated architecture documentation | Complete for current checkpoint | Keep Markdown and Word views synchronized when boundaries change | Sponsor review comments |
| 2 | CPU-core, CPU-utilization and memory characterization | In progress | Freeze benchmark scenarios, sampling schema and commands; extend instrumentation without changing DMS thresholds | Live-camera availability for final calibration/monitoring runs |
| 3 | Configurable display and processing ROI | Engineering checkpoint complete | Retain strict presentation/processing separation and repeat on target-camera data | Approve final UI defaults and target-specific driver-seat ROI |
| 4 | Per-driver eye calibration | Core implemented; product gate open | Preserve quality-gated open-eye and neutral-pose calibration; document session reset behavior | Approve production calibration duration/UX after target-data evidence |
| 5 | Driver identification and profiles | Stage 21 baseline complete; private gate waiting | Continue provider-neutral, offline, open-set architecture and public-fixture evaluation | Separate biometric/PAD consent and data; deferred privacy/security decisions |
| 6 | TI SK-AM62 platform | Planning | Define BSP, toolchain, camera, acceleration, packaging and acceptance gates | Physical board, intended camera and selected Processor SDK |
| 7 | Eye-open percentage and blink statistics | Engineering checkpoint complete | Connect reset only to a future confirmed identity-change signal | Approve display wording, reset policy and rolling-window default |
| 8 | Dependency/model inventory and release compliance | In progress | Generate deterministic source SBOM; close provenance gaps where evidence exists | Product-owner license selection and legal/release approval |

## Status rules

- **Complete** requires committed evidence and the applicable acceptance gate.
- **In progress** permits implementation and non-destructive validation but does
  not authorize a product claim.
- **Waiting** means safe autonomous preparation is exhausted and a named
  external input is required.
- Merge, release, model training, and production threshold changes remain
  excluded unless separately authorized.

## Current autonomous sequence

1. Add reproducible source-component inventory and SBOM generation.
2. Define resource benchmark scenarios and machine-readable output.
3. Define display/AOI/statistics configuration and validation behavior.
4. Define the SK-AM62 bring-up and acceptance sequence.
5. Update the architecture documents after implementation boundaries stabilize.

## Stage 24 validation checkpoint — 2026-09-04

Stage 24 configuration, processing ROI and cumulative/rolling eye/blink
statistics are implemented on `feature/stage24-display-aoi-statistics` and
validated through commit `8ce798c`. A fresh Windows x64 Release build passed
29/29 tests; a fresh MediaPipe-enabled Orin aarch64 Release build passed all 27
applicable tests. Repeated full-frame/illustrative-ROI comparisons retained
100% detection on the checksum-pinned engineering image and confirmed the ROI
path reduces work on both targets. See `docs/STAGE24_VALIDATION_REPORT.md`.

This closes the engineering gate, not the product gate. Final UI defaults,
camera-specific ROI, confirmed-driver reset integration and production claims
remain separately controlled decisions.
