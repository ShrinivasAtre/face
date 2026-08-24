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

## Production-selection expansion

Before selecting a production provider, repeat representative clips with
multiple consented adults, multiple sessions, the final visible/IR cameras,
representative mounting and distance, relevant eyewear, and intended lighting.
Keep subjects separated between calibration and held-out evaluation sets.

## Current authorization (2026-08-24)

- No suitable recordings currently exist.
- When recordings are supplied, the agent may read them and extract temporary
  frames for annotation and benchmarking.
- No special retention/deletion restriction currently applies; the user will
  explicitly state any later restriction.
- Anonymous aggregate metrics and recording checksums may be committed.
- Recordings, extracted frames, biometric annotations, and identity data remain
  outside Git by default.

