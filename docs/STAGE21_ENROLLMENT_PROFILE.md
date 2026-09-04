# Stage 21.4 enrollment and profile administration

Date: 2026-09-03

## Current increment

The separate `driver_profile_admin` executable manages a model-neutral encrypted
profile store. It is intentionally absent from the normal DMS runtime. Current
commands initialize a store, create/list/delete profiles, retain an enrollment
frame from a photograph, video, or live camera, and export the already-encrypted
portable bundle.

The store enforces 50 profiles, ten images, ten embeddings, bounded fields,
finite qualities, model-tagged embeddings, strict parsing, unique anonymous
IDs, and complete profile deletion. The import API supports reject, replace, or
new-anonymous-ID conflict behavior; CLI import workflow remains the next
increment.

## Cryptographic format

- magic and schema version are authenticated associated data;
- PBKDF2-HMAC-SHA-256 with a unique 16-byte salt and at least 600,000 iterations;
- AES-256-GCM with a new random 96-bit nonce and 128-bit authentication tag;
- Windows uses the operating-system CNG implementation and system RNG;
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

## Usage

```text
driver_profile_admin init --store=profiles.dmsid
driver_profile_admin create --store=profiles.dmsid --driver-id=driver-01 --display-name="Driver One"
driver_profile_admin list --store=profiles.dmsid
driver_profile_admin add-media --store=profiles.dmsid --driver-id=driver-01 --source=photo --input=photo.jpg --quality=0.9
driver_profile_admin add-media --store=profiles.dmsid --driver-id=driver-01 --source=video --input=enrollment.mp4 --quality=0.9
driver_profile_admin add-media --store=profiles.dmsid --driver-id=driver-01 --source=live --camera=0 --quality=0.9
driver_profile_admin export --store=profiles.dmsid --output=portable-profile-bundle.dmsid
driver_profile_admin delete --store=profiles.dmsid --driver-id=driver-01
```

These examples do not establish an approved quality threshold. Until a quality
provider is integrated, the administrator supplies diagnostic quality and the
data is retained but cannot become a production identity template.

## Validation checkpoint

Windows Release passes all 28 registered CTests. The focused profile test covers
serialization, field bounds, duplicate rejection, conflict rejection/new-ID
import, deletion, weak-KDF rejection, encrypted round trip, incorrect passphrase,
and ciphertext tampering. A CLI smoke test created, listed, exported, and
inspected a synthetic profile: source/export hashes matched and the display name
was absent from ciphertext.

## Remaining Stage 21.4 work

1. Add the CLI import/re-encryption and explicit replace/new-ID workflow.
2. Add platform-protected local-key mode; current stores use portable passphrases.
3. Add an OpenSSL 3 provider and validate byte-compatible Ubuntu/Orin bundles.
4. Integrate quality, alignment, embedding, and mandatory PAD providers after
   candidate selection; do not create production embeddings beforehand.
5. Add bounded automatic-template replacement and rollback journal.
6. Run a notified live-camera enrollment usability check.
