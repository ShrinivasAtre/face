# Stage 21 public-fixture baseline results

Date: 2026-09-03

## Scope

This checkpoint verifies offline model loading, preprocessing, embedding/PAD
outputs, and repeatable latency measurement. It is not an identity-accuracy,
presentation-attack, demographic, commercial-license, or threshold result.
No private Stage 21 participant data was used.

## External evaluation inputs

All inputs remain outside Git under the local `p21` validation roots.

| Artifact | Bytes | Digest | Source |
|---|---:|---|---|
| `face_recognition_sface_2021dec.onnx` | 38,696,353 | SHA-256 `0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79` | OpenCV Zoo `main`, acquired 2026-09-03 |
| `anti-spoof-mn3.onnx` | 12,270,179 | SHA-256 `c4c99af04603b62d7e44f6f4daeb33e0daeccc696008c0b1d62f6f5cebbb3262` | Open Model Zoo 2022.1 storage, acquired 2026-09-03 |
| `anti-spoof-mn3.onnx` | 12,270,179 | SHA-384 `6de4534964b723397b3e8c995cadcf43bc007cc2f9930b95ae25f76adccece5d1d4d058d0b15117b9e4a9f758424f92a` | Matches official OMZ model manifest |
| `lena.jpg` | 91,814 | SHA-256 `7de7ed51a1594fff247f4cae2301eceacf5313d6011e37b4a4c8733f7bb72c07` | OpenCV `4.x` sample data |
| `messi5.jpg` | 72,937 | SHA-256 `1d570e49654e84c7a943918537bd9e5e1ef82920152e147c834006e235be97c9` | OpenCV `4.x` sample data |

The existing checksum-governed YuNet detector was used for face localization.

## Windows x64 result

Environment: OpenCV 4.8.0, OpenCV DNN CPU, Release build, ten warmups and 100
timed iterations.

| Work | Output | Mean | p50 | p95 |
|---|---|---:|---:|---:|
| SFace aligned embedding | 128 dimensions | 20.153 ms | 18.713 ms | 24.232 ms |
| PAD inference, Lena crop | class0 `0.197804`, class1 `0.802196` | 9.771 ms | 9.713 ms | 11.394 ms |
| PAD inference, Messi crop | class0 `0.984818`, class1 `0.015182` | 9.357 ms | 9.499 ms | 10.970 ms |

The self-comparison cosine was `1.000000`; Lena-versus-Messi was `0.130308`.
Those are plumbing checks, not genuine/impostor operating points: the genuine
pair is the identical image, and two downloaded digital photographs are not
valid camera-captured bona-fide/PAD samples. The contradictory PAD outputs
demonstrate why a threshold or accuracy claim from these fixtures is prohibited.

## Ubuntu x64 result

A fresh checkout of commit `0e3ef41` configured and built in Ubuntu 24.04 x64
with GCC 13.3.0, CMake 3.28.3, and the distribution OpenCV 4.6.0. All 20
applicable CTests passed, including `driver_identity_matcher_test`.

All four external input SHA-256 values matched Windows. Both SFace and
`anti-spoof-mn3` failed at model execution with OpenCV DNN reporting
`Layer with requested id=-1 not found`. This is a runtime/model compatibility
failure in OpenCV 4.6.0; no latency or score is reported. The candidate cannot
claim Ubuntu support through the current distribution runtime. The next model
gate must either validate a pinned newer OpenCV build or introduce a separately
packaged provider such as ONNX Runtime, then rerun the exact hashes.

## Orin status for later Stage 21.7

The configured Orin is reachable and reports aarch64 with OpenCV 4.8.0. Copying
the exact committed Git bundle and external evaluation assets to its fresh
`~/common/p21` root requires explicit authorization for source/model transfer.
No Orin Stage 21 build or model result is claimed. That cross-platform work is
part of the later Stage 21.7 gate, outside this completed local baseline.

## Commands

```text
face_recognition_baseline --detector=<YuNet> --recognizer=<SFace> \
  --enrollment=<Lena> --genuine=<Lena> --impostor=<Messi> \
  --warmup=10 --iterations=100

face_pad_baseline --detector=<YuNet> --model=<anti-spoof-mn3> \
  --image=<public fixture> --warmup=10 --iterations=100
```

Paths are deliberately omitted from committed results. Both tools emit one
schema-1 JSON object and never save an image or embedding.

## Next evidence gate

The authorized local-baseline gate is complete. Before platform acceptance,
resolve the Ubuntu runtime compatibility and run the exact commit/model/fixture
hashes on Orin aarch64. Separately collect consented five-person
enrollment/probe/PAD data
before reporting identity or spoof accuracy. A larger representative dataset is
still required for production thresholds and a 50-driver gallery claim.
