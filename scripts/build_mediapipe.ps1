$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$MediaPipeRoot = Join-Path $RepoRoot 'third_party\mediapipe'
$BridgeRoot = Join-Path $MediaPipeRoot 'face_bridge'

if (-not (Test-Path (Join-Path $MediaPipeRoot 'WORKSPACE'))) {
    throw "MediaPipe workspace not found at $MediaPipeRoot. Run scripts\fetch_mediapipe.ps1 first."
}

if (Test-Path $BridgeRoot) {
    Remove-Item -Recurse -Force $BridgeRoot
}

New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'api') | Out-Null
New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'src') | Out-Null

Copy-Item (Join-Path $RepoRoot 'mediapipe\api\FaceMediaPipe.h') (Join-Path $BridgeRoot 'api\FaceMediaPipe.h')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\FaceMediaPipe.cpp') (Join-Path $BridgeRoot 'src\FaceMediaPipe.cpp')

@'
cc_library(
    name = "FaceMediaPipe",
    srcs = ["src/FaceMediaPipe.cpp"],
    hdrs = ["api/FaceMediaPipe.h"],
    deps = [
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
    ],
    copts = ["/DFACE_MEDIAPIPE_BUILD"],
    linkstatic = False,
    visibility = ["//visibility:public"],
)
'@ | Set-Content -Encoding UTF8 (Join-Path $BridgeRoot 'BUILD.bazel')

Push-Location $MediaPipeRoot
try {
    bazelisk build //face_bridge:FaceMediaPipe
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "MediaPipe bridge build completed."
Write-Host "Bazel output: $MediaPipeRoot\bazel-bin\face_bridge\"
