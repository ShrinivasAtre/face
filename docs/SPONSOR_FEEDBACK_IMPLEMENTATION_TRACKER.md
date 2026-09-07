# Sponsor feedback implementation tracker

Date: 2026-09-07

This tracker converts the sponsor feedback into numbered, independently
reviewable work packages. Status describes repository evidence, not a released
or safety-certified product.

| # | Work package | Status | Autonomous next action | External decision or resource |
|---:|---|---|---|---|
| 1 | Coordinated architecture documentation | Closed for current checkpoint; sponsor accepted | Keep source guide and sponsor brief synchronized when boundaries change | Reopen only for new sponsor comments |
| 2 | CPU-core, CPU-utilization and memory characterization | Windows/Orin instrumentation complete; live driver pending | Prepare repeatable live-camera comparison commands | User will notify when representative driver/camera session is available; Ubuntu x64 on hold |
| 3 | Configurable display and processing ROI | Engineering checkpoint complete; defaults approved | Preserve presentation/processing separation in later integration | Camera-specific driver-seat ROI after target session |
| 4 | Per-driver eye calibration | Core implemented; product gate open | Preserve quality-gated open-eye and neutral-pose calibration; document session reset behavior | Approve production calibration duration/UX after target-data evidence |
| 5 | Driver identification and profiles | Stage 21 baseline complete; private gate waiting | Continue provider-neutral, offline, open-set architecture and public-fixture evaluation | Separate biometric/PAD consent and data; deferred privacy/security decisions |
| 6 | TI SK-AM62 platform | On hold; portability boundary retained | Avoid target assumptions in shared C++ interfaces | User will notify when board, camera and Processor SDK are available |
| 7 | Eye-open percentage and blink statistics | Engineering checkpoint complete; five-minute window/reset policy approved | Connect reset only through a stable confirmed-identity event after Stage 21.5 authorization | Stage 21.5 runtime authorization and identity evidence |
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
