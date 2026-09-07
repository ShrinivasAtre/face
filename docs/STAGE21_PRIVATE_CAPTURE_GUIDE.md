# Stage 21 five-person identity and PAD capture kit

Date: 2026-09-07

## Gate and privacy boundary

This is a private prototype-evaluation kit, not production enrollment. Stage 20
consent does not cover identity or PAD. Obtain the separate consent recorded in
`consent_register.csv` before capturing anything. Keep this batch outside Git;
only anonymous aggregates and approved checksums may later be committed.

## Operator sequence

1. On the private data drive, initialize a new empty batch:

   ```powershell
   scripts\initialize_stage21_identity_batch.ps1 -Root D:\private\dms-stage21-five-person
   ```

2. Assign `ID01` through `ID05` without putting names in filenames or manifests.
   Keep the identity-to-person key separately under the data owner's control.
3. Review consent with each person. Complete every consent field, including
   retained images, embeddings, PAD, export/import, deletion, and automatic
   update. If any field is declined, stop that participant's corresponding test.
4. Capture ten enrollment images per participant: three forward/normal, forward
   dim, forward bright, left normal/dim, right normal/dim, and one natural
   appearance variation. Use a stable camera, include only the participant, and
   avoid unrelated identifying background content.
5. Capture at least two genuine live PAD samples and, only with explicit PAD
   consent, one print and one replay presentation. Do not interpret these four
   samples as a production PAD dataset.
6. Put files below the participant's private folder. Fill `relative_path` and
   the matching `consent_id` in `capture_manifest.csv`.
7. Validate structure and then write checksums:

   ```powershell
   python scripts\validate_stage21_identity_batch.py D:\private\dms-stage21-five-person
   python scripts\validate_stage21_identity_batch.py D:\private\dms-stage21-five-person --write-checksums
   python scripts\validate_stage21_identity_batch.py D:\private\dms-stage21-five-person
   ```

8. Notify the developer only that the batch is ready and provide its local path.
   Do not email, upload, or commit the media or identity key.

## Driver/participant instructions

- Sit in the normal driving position and look naturally rather than holding the
  eyes unusually open.
- Remove sunglasses; ordinary corrective glasses should be retained for the
  natural-variation sample if normally worn.
- For left/right samples, turn the head moderately while remaining within the
  camera view. Do not create extreme poses.
- Follow the operator's PAD directions. The software must return Unknown or
  unavailable if liveness cannot be established.
- Ask for deletion at any time; the operator must remove the profile, captures,
  embeddings and working exports associated with the anonymous ID.

## Developer evaluation sequence after handoff

1. Re-run the validator read-only and freeze approved checksums.
2. Confirm participant-disjoint enrollment/query splits and no filename leakage.
3. Run non-mutating quality, alignment, SFace and PAD diagnostics.
4. Produce per-condition and aggregate open-set/PAD results without names or raw paths.
5. Present match, ambiguity and PAD distributions for product-owner threshold review.
6. Only after threshold approval, enable persistence in the enrollment processor.
7. Exercise encrypted export/import, bounded automatic replacement/rollback and complete deletion.
8. Do not begin Stage 21.5 runtime identity publication until it is separately authorized.
