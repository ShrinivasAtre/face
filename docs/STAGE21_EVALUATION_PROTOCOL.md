# Stage 21 driver-identification evaluation protocol

## Scope

Evaluate open-set identification and mandatory presentation-attack detection
without treating a similarity score as probability. Raw images, videos,
embeddings, names, per-comparison results, and consent records remain outside
Git. Anonymous aggregates and approved model/file checksums may be committed.

## Prototype collection

For each of five separately consenting participants, collect independent
enrollment and probe sessions. Each session should include frontal, moderate
left/right/up/down pose, representative distance, normal/dim/uneven light,
visible and genuine IR where supported, glasses where applicable, and natural
appearance changes. Do not split neighboring video frames between enrollment
and evaluation.

Collect approved presentation attacks for every available display/print class:
printed enrollment photo, phone/tablet replay, and prerecorded video replay.
Masks or 3-D attacks are not claimed unless separately and safely collected.

## Partitions

- Enrollment gallery: approved captures from enrollment session only.
- Known probes: later sessions from enrolled people.
- Unknown probes: people wholly absent from the gallery.
- PAD real probes and attack probes: session/media grouped.
- Threshold-development and final-test sessions are disjoint.
- Both eyes, adjacent frames, clips, sessions, and transformations stay grouped.

Five people establish integration behavior only. Production acceptance needs a
larger, representative and statistically justified gallery/probe population.

## Recognition metrics

- Failure to detect, align, enroll, and acquire.
- Genuine/impostor similarity distributions.
- False Match Rate and False Non-Match Rate for 1:1 diagnostics.
- False Positive Identification Rate and False Negative Identification Rate for
  open-set 1:N operation at the complete 50-profile gallery size.
- Rank-1 identification, `Unknown` recall, ambiguity rate, and coverage.
- Time to stable match, identity-switch rate, and false accepts per hour.
- Calibration error only if a percentage/confidence mapping is proposed.

NIST FRTE 1:N terminology and operating-point reporting are the reference:
https://pages.nist.gov/frvt/html/frvt1N.html

## PAD metrics

- Attack Presentation Classification Error Rate (APCER).
- Bona Fide Presentation Classification Error Rate (BPCER).
- Average Classification Error Rate (ACER) as a diagnostic, not the only gate.
- Per-attack-medium and per-camera results.
- Indeterminate rate and end-to-end identity availability after PAD gating.
- Replay/print false acceptance per operating hour.

## Slices

Report by subject/session, visible versus IR, camera, lighting, pose, eyewear,
distance, source type, enrollment mode, attack medium, platform, and model.
Demographic slices require adequate consent and sample sizes; otherwise report
the evidence gap without inferring parity. NIST notes that both image quality
and algorithm choice affect demographic error differentials:
https://pages.nist.gov/frvt/html/frvt_demographics.html

## Resource metrics

Measure cold initialization, enrollment, single inference, stable-stream p50,
p95 and p99 latency, throughput, overall CPU, per-core CPU, RSS/working set,
model and gallery storage, import/export time, and sustained thermal behavior.
Use the same images, preprocessing and gallery across Windows, Ubuntu and Orin.

## Acceptance rules

- Product thresholds are not selected from public samples or the five-person
  prototype.
- No threshold is changed merely to improve one subgroup or condition.
- Missing/PAD-indeterminate observations fail closed for identity.
- A candidate cannot pass production selection without weight rights, exact
  provenance, reproducible checksum, platform results, and attack testing.
- False-match and false-non-match targets require product-owner approval after
  baseline curves and operational consequences are presented.
