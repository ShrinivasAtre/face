# Stage 21 security and privacy verification

Date: 2026-09-07  
Status: autonomous non-production checkpoint; private-data and product gates remain open

## Assets and trust boundaries

Protected assets are enrollment images, embeddings, anonymous/profile metadata,
rollback history, passphrases, protected local keys, portable exports, consent
records and the separate anonymous-ID-to-person key. The offline administrator
is a distinct trust boundary from the monitoring runtime. Model files and
captured media are untrusted inputs even when they are local.

## Threat and control matrix

| Threat | Current control | Verified evidence | Remaining gate |
|---|---|---|---|
| Store disclosure | Whole payload encrypted with AES-256-GCM; PBKDF2-HMAC-SHA-256, unique salt/nonce | Windows CNG and Linux OpenSSL round trips; ciphertext differs from plaintext | Product passphrase policy and managed key lifecycle |
| Bundle modification/wrong key | Header authenticated as AAD; authentication failure clears output | Tamper and wrong-passphrase tests on Windows/Ubuntu/Orin-class provider | Independent security review/fuzzing |
| Weak or hostile KDF header | 600,000 minimum and 10,000,000 maximum iterations | Boundary rejection tests | Review when guidance changes |
| Local key theft/reuse | Random secret protected to current Windows user by DPAPI; no plaintext sidecar | Reopen and modified-blob rejection tests | Linux device-key provider/TPM decision |
| Partial write/corruption | Sibling temporary file and atomic replacement | CLI import/store smoke evidence | Power-loss fault-injection test |
| Import collision | Explicit reject, replace or new anonymous ID; work on a copy before save | Transactional import tests | Operator UI confirmation |
| Unapproved enrollment | Alignment, quality, mandatory PAD and embedding ordered fail-closed | Mock/provider tests; diagnostic path cannot mutate | Private data and threshold approval |
| Silent template drift | Replacement retains bounded encrypted rollback history | Replace/serialize/reopen/rollback tests | Approve update rate, trigger and rollback UX |
| Dataset leakage | Private root must be outside Git; anonymous manifest and checksum validation | Initializer refuses repository paths; validator checks containment and consent IDs | Operator access control and retention audit |
| Incomplete consent | Every sensitive use has an explicit per-participant consent field | Validator rejects missing/negative consent | Approved consent wording/legal review |
| Capture substitution/tamper | SHA-256 recorded per capture | Automated validator test rejects modified bytes | Secure transfer/storage process |
| Residual deleted data | Logical profile deletion and new authenticated ciphertext replace the active store | Profile absence after erase is tested | Filesystem/SSD backups may retain old blocks; define device sanitization and backup deletion |
| Identity mispublication | Unknown/Ambiguous/Spoof/Unavailable are explicit; runtime publication absent | Matcher and enrollment tests | Stage 21.5 authorization, temporal confirmation and thresholds |

## Verification completed without private data

1. Strict profile bounds, unique IDs and model-tagged finite embeddings.
2. Authenticated encryption round trip and tamper/wrong-passphrase rejection.
3. Windows DPAPI protected-key creation, reopen and modified-blob rejection.
4. Windows/Linux bundle interoperability; Orin OpenSSL 3 profile tests.
5. Transactional import conflict behavior and logical profile deletion.
6. Bounded three-entry automatic-replacement rollback persistence.
7. Consent-first batch initialization, path containment, checksum creation and
   tamper rejection using synthetic files only.

The capture-batch and encrypted rollback tests passed 2/2 in fresh Windows
Release validation and in the dedicated Orin aarch64 `~/common/p25` checkout at
commit `d848ad4`. No participant data or camera was used.

## Important deletion limitation

`driver_profile_admin delete` removes the profile from the logical database and
atomically writes new authenticated ciphertext. It cannot promise forensic
erasure of prior filesystem blocks, SSD wear-leveling copies, backups, exported
bundles or separately retained captures. Production deletion therefore requires
a documented inventory of copies, deletion of each copy, encrypted storage with
key destruction where appropriate, and a platform-specific sanitization policy.

## Remaining external decisions and evidence

- approved consent wording and private five-person captures;
- recognition, ambiguity, quality and PAD thresholds after private evaluation;
- automatic update rate/trigger and operator rollback behavior;
- Linux device-local key mechanism and backup/export lifecycle;
- penetration/fuzz testing depth and independent security review;
- retention period, audit roles, incident response and device sanitization;
- explicit Stage 21.5, merge and release authorization.
