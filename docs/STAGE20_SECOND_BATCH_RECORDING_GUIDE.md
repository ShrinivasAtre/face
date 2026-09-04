# Stage 20 second-batch recording guide

## Purpose and authorization

This batch expands the private development evidence for eye-state availability,
IR, eyewear, occlusion, and 60-second PERCLOS windows. Local recording review
and eye-crop extraction are approved. Model training is not approved.

Record only consenting adults in a stationary vehicle or fixed indoor test
setup. Never perform scripted closures or occlusions while driving. Keep real
names and other identity data out of filenames, notes, and committed material.

## Capture contract

- Assign anonymous subject IDs `S03`, `S04`, and `S05`; do not reuse an ID for
  different people.
- Use the intended dashboard camera position and distance. Do not hand-hold or
  move the camera during a clip.
- Prefer the original 1920x1080 stream at a true 30 FPS. Keep original timing
  metadata and do not transcode, trim, stabilize, beautify, or auto-crop.
- Record visible and genuine near-IR sources as separate files. Do not simulate
  IR with a filter.
- Audio narration is useful but optional. If audio is retained, say the action
  immediately before performing it. Do not say a name.
- Each clip must be at least 90 seconds so it can contribute 60-second windows.
- Place completed files in
  `D:\work\TestClips\stage20-second-batch\incoming`.

## Four clips per subject

Replace `S03` below with the assigned anonymous subject ID.

### 1. Visible eye-state — `S03-visible-eye-state.mp4`

1. Natural forward gaze and blinking: 15 seconds.
2. Ten ordinary blinks about 3 seconds apart.
3. Closures of 0.5, 1, 2, and 3 seconds, with at least 5 seconds open between.
4. Squint for 3 seconds, reopen for 5 seconds, then partially close for 3
   seconds and reopen.
5. Natural behavior until the clip exceeds 90 seconds.

### 2. Genuine IR eye-state — `S03-ir-eye-state.mp4`

Use the intended IR camera and keep visible illumination representative of the
target setup. Repeat the visible eye-state sequence. If genuine IR is not
available, omit this clip and record that fact; do not substitute processed
visible video.

### 3. Clear-glasses/reflection — `S03-clear-glasses.mp4`

Use normal clear glasses appropriate for the participant. Natural lens
reflections are desired; do not aim a hazardous light at the eyes.

1. Forward open eyes: 15 seconds.
2. Ten ordinary blinks about 3 seconds apart.
3. Closures of 1, 2, and 3 seconds with 5 seconds open between.
4. Slowly turn the head left/right and up/down while keeping eyes open.
5. Natural behavior until the clip exceeds 90 seconds.

If clear glasses are unavailable, omit this clip rather than mislabelling it.

### 4. Occlusion and quality — `S03-eye-occlusion.mp4`

Use only safe, lightweight objects and never press on the eye.

1. Forward open eyes: 15 seconds.
2. Cover the left eye with a hand for 3 seconds; uncover for 5 seconds.
3. Cover the right eye for 3 seconds; uncover for 5 seconds.
4. Cover both eyes for 3 seconds; uncover for 5 seconds.
5. Partially cover each eye, one at a time, for 3 seconds.
6. Pass a hand briefly across the eye region five times.
7. Include representative blur/partial-face movement, then recover to the
   nominal pose.
8. Natural behavior until the clip exceeds 90 seconds.

## Handoff checklist

For every file, confirm privately that the participant consented to local DMS
development evaluation and crop extraction. Do not commit the confirmation or
recording. Update the private inventory's `consent_confirmed` and `recorded`
columns to `yes`; leave checksum and extraction columns for the agent.

After any files are present, tell the agent which anonymous subject IDs are
complete. The agent will verify media metadata, freeze checksums, run the
unchanged schema-5 benchmark, extract crops at the declared cadence, audit the
outputs, and prepare independent annotation batches.
