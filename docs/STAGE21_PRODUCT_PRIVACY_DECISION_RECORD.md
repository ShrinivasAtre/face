# Stage 21 product and privacy decision record

Date: 2026-09-03

This is an engineering decision record, not legal advice or a claim of legal
compliance. Deployment is initially limited to India and requires product/legal
review against the applicable commencement provisions of the Digital Personal
Data Protection Act, 2023 and Digital Personal Data Protection Rules, 2025.

## Approved decisions

| Topic | Decision |
|---|---|
| Purpose | Identification/profile selection only; no access control |
| Capacity | Maximum 50 drivers per device |
| Processing | Offline on the local vehicle/device |
| Portability | Profiles may move by removable media, FTP, or download |
| Enrollment sources | Still images, recorded video, and guided live capture |
| Image retention | Permitted |
| Unknown driver | Continue generic DMS; display `Unknown`; do not modify a profile |
| Profile improvement | Routine operator approval not required |
| Spoof protection | Mandatory |
| Initial platforms | Windows x64, Ubuntu x64, NVIDIA Orin aarch64 |
| Initial jurisdiction | India |
| Initial participants | Five consenting adults |

## Engineering safeguards derived from those decisions

1. Identification and retained enrollment images are optional and purpose-bound.
2. Profiles use anonymous immutable IDs; display names are separate metadata.
3. Portable exports are encrypted and integrity authenticated before transport.
   FTP is only a carrier for the already-encrypted bundle; credentials and keys
   are never stored in the bundle or repository.
4. Import rejects altered, truncated, incompatible, expired, or oversized data.
5. Device storage and exports contain model ID/version so incompatible embedding
   spaces cannot be compared.
6. Retained source images and derived embeddings have separate retention flags
   and can both be deleted through a verifiable profile-delete operation.
7. Automatic improvement cannot create, merge, or rename a profile. A sample can
   be promoted only after a stable existing match, live PAD result, quality gate,
   repeated temporal evidence, capacity bound, and rollback journal.
8. Identity logs use anonymous profile IDs and outcome codes, not images,
   embeddings, names, or unrestricted similarity histories.
9. Identity failure never disables ordinary DMS behavior.

## Deferred decisions before Stage 21.4

- export key mechanism: passphrase-derived portable key or recipient public key;
- who is an authorized enrollment/profile administrator;
- image and embedding retention duration;
- maximum templates/images per profile;
- backup and deletion propagation policy;
- display-name visibility and audit retention;
- recovery behavior after lost export credentials.

## Deferred decisions before Stage 21.5

- approved match and ambiguity thresholds from representative evidence;
- candidate confirmation duration and identity-loss timeout;
- allowed rate and rollback depth for automatic profile improvement;
- PAD threshold and supported attack classes;
- UI wording for similarity/confidence.

## Consent boundary

Consent previously obtained for fatigue/eye evaluation is not assumed to permit
identity enrollment or spoof testing. Each Stage 21 participant must separately
approve identity enrollment, retained images, derived embeddings, automatic
profile improvement, PAD capture, portability testing, and deletion testing.

Official references:

- https://www.meity.gov.in/documents/act-and-policies/digital-personal-data-protection-rules-2025-gDOxUjMtQWa
- https://www.meity.gov.in/static/uploads/2025/11/53450e6e5dc0bfa85ebd78686cadad39.pdf
