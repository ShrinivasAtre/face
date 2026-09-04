# Stage 21.4 enrollment and profile administration

Date: 2026-09-03

## Current increment

The separate `driver_profile_admin` executable manages a model-neutral encrypted
profile store. It is intentionally absent from the normal DMS runtime. Current
commands initialize a store, create/list/delete profiles, retain an enrollment
frame from a photograph, video, or live camera, and export the already-encrypted
portable bundle. Import decrypts a portable bundle with its own passphrase and
re-encrypts accepted profiles with the destination store passphrase.

The store enforces 50 profiles, ten images, ten embeddings, bounded fields,
finite qualities, model-tagged embeddings, strict parsing, unique anonymous
IDs, and complete profile deletion. The import API supports reject, replace, or
new-anonymous-ID conflict behavior, and the CLI implements all three policies.

## Cryptographic format

- magic and schema version are authenticated associated data;
- PBKDF2-HMAC-SHA-256 with a unique 16-byte salt and at least 600,000 iterations;
- AES-256-GCM with a new random 96-bit nonce and 128-bit authentication tag;
- Windows uses the operating-system CNG implementation and system RNG;
- Ubuntu/Orin-class Linux builds use OpenSSL 3 `libcrypto` and its DRBG;
- the whole database payload is encrypted, including names, images, embeddings,
  model IDs, revisions, and automatic-update metadata when added;
- wrong passwords, modified bytes, invalid sizes, weak KDF parameters, trailing
  data, and unsupported schemas fail closed;
- writes use a sibling temporary file followed by an atomic replacement.

The passphrase is read interactively with console echo disabled and is not
accepted as a command-line option, stored in the bundle, or logged. There is no
recovery key. The first implementation requires at least 12 characters; product
password policy should be reviewed separately.

The 600,000 PBKDF2-HMAC-SHA-256 baseline follows current OWASP guidance:
https://cheatsheetseries.owasp.org/cheatsheets/Password_Storage_Cheat_Sheet.html
AES-GCM provides authenticated encryption as specified by NIST SP 800-38D:
https://www.nist.gov/publications/recommendation-block-cipher-modes-operation-galoiscounter-mode-gcm-and-gmac

Bundle readers also cap the stored PBKDF2 work factor at 10,000,000 iterations
to prevent an unauthenticated bundle header from causing unbounded CPU work.

## Usage

```text
driver_profile_admin init --store=profiles.dmsid
driver_profile_admin init --store=profiles.dmsid --key-mode=local
driver_profile_admin create --store=profiles.dmsid --driver-id=driver-01 --display-name="Driver One"
driver_profile_admin create --store=profiles.dmsid --key-mode=local --driver-id=driver-01 --display-name="Driver One"
driver_profile_admin list --store=profiles.dmsid
driver_profile_admin add-media --store=profiles.dmsid --driver-id=driver-01 --source=photo --input=photo.jpg --quality=0.9
driver_profile_admin add-media --store=profiles.dmsid --driver-id=driver-01 --source=video --input=enrollment.mp4 --quality=0.9
driver_profile_admin add-media --store=profiles.dmsid --driver-id=driver-01 --source=live --camera=0 --quality=0.9
driver_profile_admin export --store=profiles.dmsid --output=portable-profile-bundle.dmsid
driver_profile_admin import --store=profiles.dmsid --input=portable-profile-bundle.dmsid --conflict=reject
driver_profile_admin import --store=profiles.dmsid --input=portable-profile-bundle.dmsid --conflict=replace
driver_profile_admin import --store=profiles.dmsid --input=portable-profile-bundle.dmsid --conflict=new-id --source-driver-id=driver-01 --new-driver-id=driver-02
driver_profile_admin delete --store=profiles.dmsid --driver-id=driver-01
driver_profile_admin diagnose-media --input=face.jpg --detector=yunet.onnx --recognizer=sface.onnx --pad-model=anti-spoof-mn3.onnx
```

`reject` and `replace` operate transactionally on all profiles in the imported
bundle. `new-id` imports one explicitly selected profile. Import failures leave
the destination store unchanged.

`--key-mode=local` uses a random store secret protected by Windows DPAPI for the
current OS user. The protected sidecar defaults to `<store>.key`; it contains no
plaintext key. Local-key exports prompt for a new portable passphrase and
re-encrypt the database, so the exported bundle can be copied to another device.
The optional `--key-file=<path>` selects a different protected sidecar location.

These examples do not establish an approved quality threshold. Until a quality
provider is integrated, the administrator supplies diagnostic quality and the
data is retained but cannot become a production identity template.

## Validation checkpoint

Windows Release passes all 25 tests registered on the isolated Stage 21 branch.
The focused profile test covers
serialization, field bounds, duplicate rejection, conflict rejection/new-ID
import, deletion, weak-KDF rejection, encrypted round trip, incorrect passphrase,
and ciphertext tampering. CLI smoke tests created, listed, exported, and
inspected a synthetic profile, then exercised transactional reject, replace,
and new-ID imports with different source/destination passphrases. Rejected
conflicts left the destination byte-for-byte unchanged, successful imports were
re-encrypted, and the final profile list was correct.

The protected-local-key test creates and reopens a DPAPI blob, verifies the
recovered random secret, and rejects a modified blob. A CLI smoke test also
initialized and reopened a local-key store, exported it under a portable
passphrase, imported it into a separate passphrase store, and recovered the
expected profile. DPAPI operations require access to the Windows user profile.

Ubuntu 24.04 with OpenSSL 3.0.13 passes all 23 registered tests. Bidirectional
compatibility is verified: Ubuntu opens a Windows CNG-produced bundle and
Windows opens an Ubuntu OpenSSL-produced bundle with the expected profile data.
The same provider is intended for Orin, but Orin execution remains a separate
platform gate.

## Remaining Stage 21.4 work

1. Validate the OpenSSL 3 provider and bundle compatibility on Orin.
2. Complete the in-progress quality, alignment, embedding, and mandatory PAD
   integration. The provider-neutral processor and fail-closed gate ordering
   are implemented; OpenCV candidate adapters and administrator wiring remain.
   No production embedding may be committed before threshold approval.
3. Add bounded automatic-template replacement and rollback journal.
4. Run a notified live-camera enrollment usability check.

## Enrollment processing safety contract

`DriverEnrollmentProcessor` now owns the model-neutral orchestration boundary:

1. validate the source image view;
2. require a valid aligned face;
3. require a finite quality score and apply the configured diagnostic gate;
4. require PAD state `Live`—`Spoof`, `Indeterminate`, and unavailable all stop;
5. only then request a finite, non-empty, correctly model-tagged embedding;
6. return `ThresholdApprovalRequired` instead of accepting enrollment while
   product thresholds remain unapproved.

The current focused test uses deterministic mock providers and verifies call
ordering plus fail-closed behavior. It does not approve a quality or PAD
threshold and does not add a production embedding to a driver profile.

## OpenCV evaluation adapters

The OpenCV adapter library now provides:

- YuNet detection plus SFace five-point alignment;
- SFace 128-dimensional evaluation embeddings with an explicit model ID;
- a deterministic diagnostic-only exposure/sharpness quality score;
- `anti-spoof-mn3` preprocessing and two-class softmax diagnostics;
- fail-closed construction and inference diagnostics for absent or invalid
  model assets.

`diagnose-media` is intentionally non-mutating: it requires no profile store,
prints `diagnostic_only:true` and `enrollment_allowed:false`, and never writes an
image or embedding. With the external `D:\work\p21` evaluation models and public
`lena.jpg` fixture on Windows/OpenCV 4.8, it aligned a face, reported diagnostic
quality `0.956888`, retained PAD as `Indeterminate` because thresholds are not
approved, and produced a 128-dimensional SFace evaluation embedding. These
numbers are plumbing evidence, not accuracy or threshold evidence.

The adapter focused test passes on Windows/OpenCV 4.8 and Ubuntu/OpenCV 4.6. It
checks bounded quality output and fail-closed missing-model behavior. Ubuntu
source compatibility does not override the already recorded OpenCV 4.6 model
execution limitation; actual candidate inference remains a Windows baseline
until the Ubuntu runtime is upgraded or replaced.
