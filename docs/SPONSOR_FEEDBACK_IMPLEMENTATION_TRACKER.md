# Sponsor feedback implementation tracker

Date: 2026-09-07

This tracker converts the sponsor feedback into numbered, independently
reviewable work packages. Status describes repository evidence, not a released
or safety-certified product.

| # | Work package | Status | Autonomous next action | External decision or resource |
|---:|---|---|---|---|
| 1 | Coordinated architecture documentation | Closed for current checkpoint; sponsor accepted | Keep Markdown, source guide and sponsor brief synchronized | Reopen only for new sponsor comments |
| 2 | CPU-core, CPU-utilization and memory characterization | Windows/Orin complete; live-driver runbook ready; Ubuntu deferred | Execute paired target-camera runs when physical session is available | User will notify when driver/camera are ready; replacement Ubuntu setup remains on hold |
| 3 | Configurable display and processing ROI | Engineering checkpoint complete; product defaults approved | Retain strict presentation/processing separation and repeat on target-camera data | Camera-specific driver-seat ROI after live view is available |
| 4 | Per-driver eye calibration | Core implemented; product gate open | Preserve quality-gated open-eye and neutral-pose calibration; document session reset behavior | Approve production calibration duration/UX after target-data evidence |
| 5 | Driver identification and profiles | Stage 21 baseline complete; private gate waiting | Continue provider-neutral, offline, open-set architecture and public-fixture evaluation | Separate biometric/PAD consent and data; deferred privacy/security decisions |
| 6 | TI SK-AM62 platform | On hold; portability plan complete | Keep the shared C++ core free of target assumptions | User will notify when board, intended camera and Processor SDK are available |
| 7 | Eye-open percentage and blink statistics | Engineering checkpoint complete; wording, five-minute default and reset policy approved | Connect reset only to a future stable confirmed-identity signal | Stage 21.5 authorization and identity evidence |
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

The follow-up through commit `2ab2a95` adds explicit headless live-camera
benchmark input and display-only `full`, `face`, `eyes` and `mouth` focus modes.
The focus selection is applied after all monitoring work and uses a blank view
when its semantic feature is unavailable. Windows passed 30/30 registered
Release tests and Orin passed 28/28 applicable tests. Orin device 0 sustained
30.01 FPS for 900 YuNet frames with resource sampling, but the unattended
camera view contained no driver, so calibration/eye/event evidence is correctly
not claimed. Windows exposed no physical camera at this checkpoint.

## Stage 23 into Stage 24 integration — 2026-09-07

The completed Stage 23 resource-instrumentation line is integrated into the
Stage 24 development branch. Conflict resolution preserves Stage 24 live-camera,
presentation configuration, processing ROI and eye/blink statistics together
with Stage 23's lower-priority sampler, diagnostic recalibration control and
final resource evidence. A fresh Windows x64 Release build passed 30/30 tests.
Orin regression is the remaining integration validation step; `main` is not
changed by this feature-branch integration.
