# Stage 19 Recording Guide

## Is one person's recording useful?

Yes. A controlled recording of the user's face is sufficient for bring-up,
pipeline verification, latency measurement, annotation-tool validation, and
finding obvious regressions. It is not sufficient for production model
selection because it cannot measure subject variation or demographic,
eyewear, anatomy, and behavior robustness.

Start with one subject and the intended camera. Add consented subjects and the
intended near-IR camera later before the Stage 19 selection gate.

## Capture settings

- Record the original camera stream without beauty filters, stabilization,
  virtual backgrounds, auto-cropping, or face overlays.
- Prefer 1920 x 1080 or the intended product resolution at a true 30 FPS.
- Use a fixed camera position at the intended dashboard distance and angle.
- Keep timestamps/frame rate metadata. Do not transcode unless necessary.
- Record visible-light and near-IR as separate clips when both cameras exist.
- Use anonymous filenames such as `subject01_session01_visible.mp4`.
- Do not upload recordings to GitHub.

## Initial controlled session

Record separate clips so annotations remain unambiguous:

1. **Neutral baseline (30 s):** face forward, eyes naturally open, normal
   breathing, minimal movement.
2. **Blink clip (45 s):** 10 deliberate single blinks spaced about 3 s apart,
   then 15 s of natural behavior. Say the blink number aloud if audio is
   acceptable, or keep a written timing log.
3. **Eye closure (45 s):** close both eyes for approximately 0.5, 1, 2, and
   3 seconds with open intervals; repeat once.
4. **Partial eye opening (30 s):** normal open, squint, partially closed, and
   fully closed states, held for about 3 s each.
5. **Head pose (60 s):** neutral; turn left and right; look up and down; tilt
   left and right. Hold each endpoint for 3 s and return to neutral between
   movements. Repeat twice without blinking deliberately.
6. **Mouth/yawn (45 s):** mouth closed, normal speech, deliberate mouth open,
   and three simulated yawns of at least 2 s. Label these as simulated yawns.
7. **Occlusion (45 s):** briefly cover one eye, both eyes, mouth, and part of
   the face with a hand; include drinking from a cup if safe and convenient.
8. **Distance/partial face (45 s):** nominal position, closer, farther, then
   move partly out of frame on each side.
9. **Natural combined behavior (2 min):** read or watch something naturally;
   include ordinary head and eye movement without scripted events.

Repeat the session with clear glasses and sunglasses when available. For
visible light, repeat neutral/blink/closure/head-pose clips in daylight, cabin
lighting, low light, and an illumination transition. Never perform this capture
while driving a moving vehicle.

## Three supplemental clips to request from colleagues

These three clips are the highest-priority additions to the initial session.
Share the instructions below unchanged with each consenting participant. Record
each item as a separate file while seated safely in a stationary vehicle or
other fixed test setup. Never record these actions while driving a moving
vehicle.

Before each clip:

- Fix the camera at the intended dashboard position and keep it stationary.
- Keep the entire face visible at the normal driver distance. Do not hold or
  move the camera by hand.
- Prefer the original 1920 x 1080, 30 FPS camera stream with no beauty filter,
  stabilization, auto-crop, virtual background or face overlay.
- Keep audio enabled. Say `START`, then narrate every action immediately before
  doing it, and say `END` after returning to neutral. Do not speak during an
  eye closure unless the instruction specifically calls for narration.
- Use an anonymous filename such as the example shown; do not put the person's
  name in the filename.

### 1. Clear-glasses eye test — approximately 90 seconds

Example filename: `subject01_session01_glasses_eye.mp4`.

Wear ordinary clear prescription or non-prescription glasses for the entire
clip. Do not deliberately reposition them during the test.

1. Face forward with eyes naturally open for 10 seconds.
2. Perform 10 deliberate single blinks, spaced approximately 3 seconds apart.
   Say `blink 1` through `blink 10` immediately before each blink.
3. Say `half-second closure`, close both eyes for about 0.5 seconds, then reopen
   for 3 seconds.
4. Repeat with narrated closures of 1, 2 and 3 seconds, reopening for at least
   3 seconds after each closure.
5. Say `squint`, squint for 3 seconds, then return to normally open eyes.
6. Say `partial closure`, hold both eyes partially closed for 3 seconds, then
   reopen normally.
7. Finish with 10 seconds of natural looking and blinking.

Lens reflections are useful test data. Do not tilt or remove the glasses merely
to eliminate normal reflections. If available, record a separate sunglasses
clip using the same sequence; do not combine clear glasses and sunglasses in
one file.

### 2. Low-light and illumination-transition test — 60 to 90 seconds

Example filename: `subject01_session01_low_light_transition.mp4`.

Use the same fixed camera and position as the normal-light recording. Change
only the ambient illumination; do not change camera settings manually during
the clip.

1. Begin in ordinary room or cabin lighting, facing forward with natural
   blinking for 10 seconds.
2. Say `going dark`, then gradually reduce the ambient light over approximately
   10 seconds.
3. Remain in representative low light for 20 to 30 seconds. During this period,
   perform five deliberate blinks spaced about 3 seconds apart, saying
   `low-light blink 1` through `low-light blink 5` before each blink.
4. Say `going bright`, then gradually restore the original light over
   approximately 10 seconds.
5. Finish facing forward with natural blinking for 10 seconds.

Do not look directly into a lamp, flash a bright light into the eyes, or create
an unsafe dark environment. Record near-IR as a separate original-camera clip
when an IR camera is available; do not simulate IR with a visible-light filter.

### 3. Eye gaze versus head movement — approximately 90 seconds

Example filename: `subject01_session01_gaze_head.mp4`.

This clip must separate eye-only motion from head-only motion. Keep both eyes
open naturally and avoid deliberate blinking during each held direction.

1. Face forward and look toward the camera for 10 seconds.
2. Keep the head still. Move only the eyes to look left, right, up and down,
   returning to center after every direction. Say the direction before moving,
   hold it for 2 seconds and repeat each direction five times.
3. Rest in the centered neutral position for 5 seconds.
4. Keep the eyes approximately forward relative to the face. Move only the head
   left, right, up and down, returning to neutral after every direction. Say the
   direction before moving, hold each endpoint for 2 seconds and repeat each
   direction three times.
5. Finish in a centered neutral pose for 10 seconds.

If an eye-only direction is difficult to maintain, make the movement smaller
rather than turning the head. If the participant must blink, blink normally and
continue; do not repeat or edit the recording.

## Production-selection expansion

Before selecting a production provider, repeat representative clips with
multiple consented adults, multiple sessions, the final visible/IR cameras,
representative mounting and distance, relevant eyewear, and intended lighting.
Keep subjects separated between calibration and held-out evaluation sets.

## Current authorization (updated 2026-08-26)

- A one-subject visible-light development set is available locally on the
  authorized Windows and Orin systems. Additional colleague and production-
  representative recordings are expected later.
- When recordings are supplied, the agent may read them and extract temporary
  frames for annotation and benchmarking.
- No special retention/deletion restriction currently applies; the user will
  explicitly state any later restriction.
- Anonymous aggregate metrics and recording checksums may be committed.
- Recordings, extracted frames, biometric annotations, and identity data remain
  outside Git by default.
