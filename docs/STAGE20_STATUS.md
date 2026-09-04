# Stage 20 tracked status

Date: 2026-09-03

| # | Step | Status | Current evidence / exit condition |
|---:|---|---|---|
| 1 | Establish isolated feature branch and private-data boundary | Complete | `feature/stage20-accuracy-gate`; private rows/media excluded |
| 2 | Validate unchanged Windows, Ubuntu, and Orin baselines | Complete | Accepted cross-platform checkpoint |
| 3 | Correct verified pose/gaze semantics without threshold changes | Complete | Commit `536f1a4` and cross-platform tests |
| 4 | Rerun and score six adjudicated event clips | Complete | 8,501 frames; anonymous event results retained |
| 5 | Extract/audit first eye-ROI set and prepare independent batches | Complete | 2,828 crops; training-readiness rejection retained |
| 6 | Complete two independent dense label passes and adjudication | Complete | 2,828 labels; 137 adjudicated rows; zero validation errors |
| 7 | Score dense state, coverage, confusion, and duration | Complete | 98.50% conditional and 86.51% end-to-end accuracy |
| 8 | Diagnose unknown-state and slice failures | Complete | All evaluable unknowns isolated to recovering/low-confidence policy states |
| 9 | Score fixed-window PERCLOS | Partial | Scorer/test complete; only C04 is long enough, so broader evidence is pending |
| 10 | Prepare second private recording/ingestion batch | Complete | C07-C18 inventory, safety guide, freeze/checksum tooling, tests |
| 11 | Record, ingest, extract, audit, and double-label second batch | Waiting for data | Requires consented S03-S05 recordings |
| 12 | Decide/train/benchmark eye-ROI option and close Stage 20 | Blocked by gate | Requires Step 11 readiness plus separate training approval; merge/release not authorized |

## Current decision

Keep `stage20-approved-2026-08-28` unchanged. Classification is strong when the
temporal eye state is available, but overall/IR availability, explicit
occlusion recall, subject diversity, and long-window coverage do not support a
production claim. The eye-ROI benchmark condition is triggered, but training
is premature and unauthorized until Step 11 completes.
