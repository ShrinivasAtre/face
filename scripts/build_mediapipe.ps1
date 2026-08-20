$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$MediaPipeRoot = Join-Path $RepoRoot 'third_party\mediapipe'
$BridgeRoot = Join-Path $MediaPipeRoot 'face_bridge'
$TasksCoreBuild = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\BUILD'
$TasksCoreSrc = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\task_runner.cc'
$TasksLoggingBuild = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\logging\BUILD'
$TasksDummyLogger = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\logging\tasks_dummy_logger.h'

if (-not (Test-Path (Join-Path $MediaPipeRoot 'WORKSPACE'))) {
    throw "MediaPipe workspace not found at $MediaPipeRoot. Run scripts\fetch_mediapipe.ps1 first."
}

function Write-Utf8NoBom([string]$Path, [string]$Content) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

function Remove-LineContaining([string]$Path, [string]$Needle, [string]$Message) {
    $content = Get-Content -Raw $Path
    if ($content.Contains($Needle)) {
        Write-Host $Message
        $lines = Get-Content $Path | Where-Object { -not $_.Contains($Needle) }
        Write-Utf8NoBom $Path (($lines -join [Environment]::NewLine) + [Environment]::NewLine)
    }
}

# MediaPipe v0.10.33 OSS checkout is missing internal analytics protos. Mirror
# the Linux compatibility patch exactly for the Face Landmarker path.
Remove-LineContaining $TasksCoreBuild '//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto' 'Applying MediaPipe v0.10.33 task_runner analytics compatibility patch...'
Remove-LineContaining $TasksCoreSrc 'mediapipe/util/analytics/mediapipe_logging_enums.pb.h' 'Removing unused task_runner analytics enum include...'
Remove-LineContaining $TasksLoggingBuild '":logging_client"' 'Removing dummy logger dependency on unavailable analytics logging client...'
Remove-LineContaining $TasksDummyLogger 'mediapipe/tasks/cc/core/logging/logging_client.h' 'Removing unused analytics logging_client include from dummy logger...'

if ((Get-Content -Raw $TasksCoreBuild).Contains('//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto') -or
    (Get-Content -Raw $TasksCoreSrc).Contains('mediapipe/util/analytics/mediapipe_logging_enums.pb.h')) {
    throw 'MediaPipe analytics compatibility patch did not apply cleanly.'
}

if (Test-Path $BridgeRoot) {
    Remove-Item -Recurse -Force $BridgeRoot
}

New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'api') | Out-Null
New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'src') | Out-Null

Copy-Item (Join-Path $RepoRoot 'mediapipe\api\FaceMediaPipe.h') (Join-Path $BridgeRoot 'api\FaceMediaPipe.h')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\FaceMediaPipe.cpp') (Join-Path $BridgeRoot 'src\FaceMediaPipe.cpp')

$BridgeBuild = @'
cc_library(
    name = "FaceMediaPipe_impl",
    srcs = ["src/FaceMediaPipe.cpp"],
    hdrs = ["api/FaceMediaPipe.h"],
    deps = [
        "//mediapipe/framework/formats:image_frame",
        "//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
    ],
    copts = ["/DFACE_MEDIAPIPE_BUILD"],
    alwayslink = True,
    visibility = ["//visibility:private"],
)

cc_binary(
    name = "FaceMediaPipe.dll",
    deps = [":FaceMediaPipe_impl"],
    linkshared = True,
    linkstatic = True,
    visibility = ["//visibility:public"],
)
'@

Write-Utf8NoBom (Join-Path $BridgeRoot 'BUILD.bazel') $BridgeBuild

$BazelArgs = @('build')

# TensorFlow in MediaPipe v0.10.33 only ships requirements lock files for
# Python 3.9-3.12. New Windows hosts may otherwise default to 3.14, which
# fails during repository initialization. Pin the hermetic Python repository
# to the newest version supported by this checkout.
$BazelArgs += '--repo_env=HERMETIC_PYTHON_VERSION=3.12'
Write-Host 'Using hermetic Python 3.12 for MediaPipe/TensorFlow repositories'

# Bazel's Java downloader may time out on GitHub release assets even when
# PowerShell can download them. Let Bazel reuse any matching archives that were
# downloaded manually into TEMP before it attempts the network.
if ($env:TEMP -and (Test-Path $env:TEMP)) {
    Write-Host "Using Bazel distdir: $env:TEMP"
    $BazelArgs += "--distdir=$env:TEMP"
}

$BazelArgs += '//face_bridge:FaceMediaPipe.dll'

Push-Location $MediaPipeRoot
try {
    & bazelisk @BazelArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Bazel build failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}

$DllPath = Join-Path $MediaPipeRoot 'bazel-bin\face_bridge\FaceMediaPipe.dll'
if (-not (Test-Path $DllPath)) {
    throw "Bazel reported success but $DllPath was not produced."
}

Write-Host ""
Write-Host "MediaPipe bridge build completed successfully."
Write-Host "DLL: $DllPath"
