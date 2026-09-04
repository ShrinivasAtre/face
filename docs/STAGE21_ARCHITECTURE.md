# Stage 21 driver-identification architecture

## User view

```mermaid
flowchart LR
    A[Administrator obtains consent] --> B[Enroll photo, video, or live capture]
    B --> C[Quality and spoof checks]
    C --> D[Encrypted local driver profile]
    D --> E[Export encrypted portable bundle]
    F[Driver enters camera view] --> G[Offline live and identity check]
    G --> H{Result}
    H -->|confirmed| I[Driver profile selected]
    H -->|unknown or ambiguous| J[Generic DMS continues]
    H -->|spoof or unavailable| J
```

## Developer view

```mermaid
flowchart LR
    CAM[Camera / recorded input] --> DET[Face detector]
    DET --> ALIGN[Five-point alignment and quality]
    ALIGN --> PAD[PresentationAttackProvider]
    ALIGN --> EMB[FaceEmbeddingProvider]
    STORE[DriverTemplateStore] --> MATCH[DriverIdentityMatcher]
    EMB --> MATCH
    PAD --> MATCH
    MATCH --> FSM[Identity session confirmation FSM]
    FSM --> OBS[Observation RecognitionValue]
    OBS --> DMS[Profile selection only]
    DMS --> SAFE[Generic DMS always remains active]
    ADMIN[Separate enrollment/profile utility] --> STORE
    STORE --> PORT[Encrypted portable profile bundle]
```

The detector/alignment, embedding, PAD, storage, matcher, and temporal session
logic are independently replaceable and independently measurable. Only the
provider layer understands model-specific preprocessing or output topology.

## Platform independence boundary

```mermaid
flowchart TB
    subgraph Independent[Platform-independent C++17]
      TYPES[Semantic identity/PAD contracts]
      MATCHER[Cosine gallery matcher]
      SESSION[Confirmation and identity-change FSM]
      FORMAT[Versioned logical profile/export schema]
      POLICY[Unknown, ambiguity, retention and audit policy]
    end
    subgraph Specific[Platform/provider-specific]
      CAMERA[Camera and media decoding]
      INFER[OpenCV DNN / later execution provider]
      KEY[DPAPI, Linux key store, TPM or export key]
      FS[Permissions and atomic filesystem operations]
      ACCEL[CPU, CUDA/TensorRT or target accelerator]
    end
    Specific --> Independent
```

## Initial technology register

| Block | Technology/version | License/source | Status |
|---|---|---|---|
| Application | C++17 and CMake | Project license | Retained |
| Image/inference API | OpenCV 4.8.0 Windows/Orin; Ubuntu 4.6.0 baseline | Apache-2.0, https://opencv.org | Retained |
| Face detector | Existing YuNet `2026may` asset | Existing checksum/provenance contract | Baseline only |
| Recognition candidate | OpenCV Zoo SFace `2021dec` ONNX | Directory states Apache-2.0; weight/training provenance remains a release blocker | Evaluation only |
| PAD candidate | Open Model Zoo `anti-spoof-mn3`, MobileNetV3, 128x128 | Original model stated MIT; trained on CelebA-Spoof | Evaluation candidate |
| Excluded recognition pack | InsightFace public model packs | Non-commercial research only | Not packaged or selected |
| Profile cryptography | Provider interface; primitive/key choice deferred | Must use reviewed platform crypto | Design only |

The model register records exact hashes after acquisition. A source-code license
does not by itself establish rights to redistribute weights or their training
data derivatives.
