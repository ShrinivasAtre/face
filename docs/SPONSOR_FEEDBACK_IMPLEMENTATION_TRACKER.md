# Sponsor feedback implementation tracker

Date: 2026-09-03

This tracker converts the sponsor feedback into numbered, independently
reviewable work packages. Status describes repository evidence, not a released
or safety-certified product.

| # | Work package | Status | Autonomous next action | External decision or resource |
|---:|---|---|---|---|
| 1 | Coordinated architecture documentation | Complete for current checkpoint | Keep Markdown and Word views synchronized when boundaries change | Sponsor review comments |
| 2 | CPU-core, CPU-utilization and memory characterization | Windows/Orin complete; Ubuntu deferred | Preserve the accepted schema while collecting live-camera Windows/Orin comparison evidence | Live-camera availability; replacement Ubuntu setup for the deferred generic-Linux check |
| 3 | Configurable display and processing ROI | Specification in progress | Define provider-neutral configuration, validation and safe fallback behavior | Approve final UI defaults and whether ROI is display-only or processing-constraining |
| 4 | Per-driver eye calibration | Core implemented; product gate open | Preserve quality-gated open-eye and neutral-pose calibration; document session reset behavior | Approve production calibration duration/UX after target-data evidence |
| 5 | Driver identification and profiles | Stage 21 baseline complete; private gate waiting | Continue provider-neutral, offline, open-set architecture and public-fixture evaluation | Separate biometric/PAD consent and data; deferred privacy/security decisions |
| 6 | TI SK-AM62 platform | Planning | Define BSP, toolchain, camera, acceleration, packaging and acceptance gates | Physical board, intended camera and selected Processor SDK |
| 7 | Eye-open percentage and blink statistics | Core partially implemented | Specify cumulative/session and rolling-window contracts and quality coverage | Approve display/reset policy and rolling-window default |
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
