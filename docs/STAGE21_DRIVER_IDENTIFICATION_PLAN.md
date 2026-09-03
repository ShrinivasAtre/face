# Stage 21 — offline driver identification

Date: 2026-09-03

## Objective

Add consented, offline, open-set identification for at most 50 enrolled drivers.
Identification is for naming/profile selection only. It does not grant access,
prove legal identity, suppress DMS monitoring, or alter a safety event.

Cigarette, drink, and other object/context events are excluded and move to a
separate Stage 21B gate.

## Authorized scope

The product owner authorized Steps 21.0--21.3: documentation, architecture,
interfaces, tests, private local evaluation, and pretrained-model benchmarking.
Training, fine-tuning, product threshold approval, merge, and release remain
excluded.

## Sequence and status

| # | Work package | Status | Exit condition |
|---:|---|---|---|
| 21.0 | Product/privacy contract | Complete for baseline | Decisions and deferred approvals recorded |
| 21.1 | Provider-neutral architecture/interfaces | Implemented | Interfaces and deterministic matcher test pass |
| 21.2 | Evaluation/data protocol | Complete for prototype | Leakage-safe protocol and metrics recorded |
| 21.3a | Model/license survey | Complete | Candidate, exclusion, and provenance matrix recorded |
| 21.3b | Local pretrained baseline | Partial | Windows measured; Ubuntu OpenCV 4.6 incompatibility recorded; Orin pending |
| 21.3c | Five-person private baseline | Waiting for data | Specific identity/PAD consent and enrollment captures available |
| 21.4 | Enrollment application | Not authorized | Separate implementation approval after baseline review |
| 21.5 | Runtime integration/session FSM | Not authorized | Separate implementation approval after threshold review |
| 21.6 | Security/privacy verification | Planned | Threat, deletion, portability, and key tests pass |
| 21.7 | Windows/Ubuntu/Orin acceptance | Planned | Cross-platform accuracy/resource gates pass |
| 21.8 | Merge/release | Not authorized | Explicit product-owner approval |

## Non-negotiable behavior

- Mandatory presentation-attack detection (PAD) gates every identity result.
- `Spoof` and indeterminate PAD results cannot yield a driver identity.
- No face, empty gallery, incompatible model, corrupt profile, and stale result
  are explicit unavailable states, never an implicit match.
- Below-threshold results are `Unknown`; near-tied results are `Ambiguous`.
- A single embedding comparison produces only a `Candidate`. A later temporal
  session FSM must confirm it before publishing `Matched`.
- Similarity is not displayed as a probability. A calibrated percentage may be
  added only after representative calibration evidence and approval.
- Generic DMS operation continues for unknown, spoofed, or unavailable identity.
- Raw identity material and per-person results remain outside Git.

## Acceptance gate for the authorized increment

- Requirements and privacy decisions reflect the approved product behavior.
- User, developer, and platform-boundary diagrams identify trust boundaries.
- C++ interfaces expose no provider-specific SDK types.
- Matching rejects invalid configuration, incompatible embeddings, excessive
  galleries, ambiguous matches, and non-live presentations.
- Candidate weights have explicit source/version/hash/license records. Unclear
  training-data or weight rights prevent production selection.
- Public-fixture baseline reports accuracy semantics, latency, and limitations.
- No production threshold, training, live identity database, or release claim is
  made from the five-person prototype.

## Stage 20 relationship

Stage 20 remains open for its second subject/session batch and production eye
accuracy evidence. Stage 21 shares only the provider-neutral scheduler and face
observation boundary; it does not reinterpret Stage 20 results or thresholds.
