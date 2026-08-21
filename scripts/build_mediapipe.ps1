$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$MediaPipeRoot = Join-Path $RepoRoot 'third_party\mediapipe'
$BridgeRoot = Join-Path $MediaPipeRoot 'face_bridge'
$OpenCvAdapterRoot = Join-Path $MediaPipeRoot 'face_windows_opencv'
$TasksCoreBuild = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\BUILD'
$TasksCoreSrc = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\task_runner.cc'
$TasksLoggingBuild = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\logging\BUILD'
$TasksDummyLogger = Join-Path $MediaPipeRoot 'mediapipe\tasks\cc\core\logging\tasks_dummy_logger.h'
$GpuServiceSrc = Join-Path $MediaPipeRoot 'mediapipe\gpu\gpu_service.cc'
$Api3PatchScript = Join-Path $PSScriptRoot 'patch_mediapipe_windows_api3.ps1'

if (-not (Test-Path (Join-Path $MediaPipeRoot 'WORKSPACE'))) {
    throw "MediaPipe workspace not found at $MediaPipeRoot. Run scripts\fetch_mediapipe.ps1 first."
}

# Keep all Windows compatibility patches inside the normal build flow so a
# fresh clone needs only fetch_mediapipe.ps1 followed by build_mediapipe.ps1.
if (-not (Test-Path $Api3PatchScript)) {
    throw "Required Windows MediaPipe API3 patch script not found: $Api3PatchScript"
}
& $Api3PatchScript

function Write-Utf8NoBom([string]$Path, [string]$Content) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

function Normalize-Utf8NoBom([string]$Path) {
    $content = [System.IO.File]::ReadAllText($Path)
    Write-Utf8NoBom $Path $content
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

# MediaPipe declares kGpuService with ABSL_CONST_INIT in gpu_service.h but the
# pinned v0.10.33 definition omits it. Newer MSVC rejects this declaration /
# definition mismatch. Keep both sides consistent without changing semantics.
$GpuServiceContent = Get-Content -Raw $GpuServiceSrc
$GpuServiceOld = 'const GraphService<GpuResources> kGpuService('
$GpuServiceNew = 'ABSL_CONST_INIT const GraphService<GpuResources> kGpuService('
if ($GpuServiceContent.Contains($GpuServiceOld) -and -not $GpuServiceContent.Contains($GpuServiceNew)) {
    Write-Host 'Applying MediaPipe GPU service constinit compatibility patch...'
    $GpuServiceContent = $GpuServiceContent.Replace($GpuServiceOld, $GpuServiceNew)
    Write-Utf8NoBom $GpuServiceSrc $GpuServiceContent
}

# Older revisions of this script used Set-Content -Encoding UTF8, which adds a
# UTF-8 BOM under Windows PowerShell 5.1. Normalize every MediaPipe file we may
# patch on every run so an existing checkout is repaired automatically.
Normalize-Utf8NoBom $TasksCoreBuild
Normalize-Utf8NoBom $TasksCoreSrc
Normalize-Utf8NoBom $TasksLoggingBuild
Normalize-Utf8NoBom $TasksDummyLogger
Normalize-Utf8NoBom $GpuServiceSrc

if ((Get-Content -Raw $TasksCoreBuild).Contains('//mediapipe/util/analytics:mediapipe_logging_enums_cc_proto') -or
    (Get-Content -Raw $TasksCoreSrc).Contains('mediapipe/util/analytics/mediapipe_logging_enums.pb.h')) {
    throw 'MediaPipe analytics compatibility patch did not apply cleanly.'
}
if (-not (Get-Content -Raw $GpuServiceSrc).Contains($GpuServiceNew)) {
    throw 'MediaPipe GPU service constinit compatibility patch did not apply cleanly.'
}

# MediaPipe v0.10.33's bundled @windows_opencv repository is hard-coded for
# OpenCV 3.4.10 / VC15. Reuse the OpenCV 4.8.0 installation already used by
# this project instead. OPENCV_ROOT can override auto-detection on another host.
$OpenCvRoot = $env:OPENCV_ROOT
if (-not $OpenCvRoot) {
    $OpenCvCandidates = @(
        'C:\opencv-4.8.0-src\install',
        'C:\opencv\build'
    )
    foreach ($candidate in $OpenCvCandidates) {
        if ((Test-Path (Join-Path $candidate 'include\opencv2\core\version.hpp')) -and
            (Test-Path (Join-Path $candidate 'lib\opencv_world480.lib')) -and
            (Test-Path (Join-Path $candidate 'bin\opencv_world480.dll'))) {
            $OpenCvRoot = $candidate
            break
        }
    }
}

if (-not $OpenCvRoot) {
    throw 'OpenCV 4.8.0 install not found. Set OPENCV_ROOT to a directory containing include\opencv2, lib\opencv_world480.lib and bin\opencv_world480.dll.'
}

$OpenCvRoot = (Resolve-Path $OpenCvRoot).Path
$OpenCvHeader = Join-Path $OpenCvRoot 'include\opencv2\core\version.hpp'
$OpenCvLib = Join-Path $OpenCvRoot 'lib\opencv_world480.lib'
$OpenCvDll = Join-Path $OpenCvRoot 'bin\opencv_world480.dll'
foreach ($required in @($OpenCvHeader, $OpenCvLib, $OpenCvDll)) {
    if (-not (Test-Path $required)) {
        throw "Required OpenCV 4.8.0 file not found: $required"
    }
}
Write-Host "Using OpenCV 4.8.0 from: $OpenCvRoot"

# Build a generated local Bazel repository with the exact target name
# MediaPipe expects: @windows_opencv//:opencv. This leaves the OpenCV install
# itself untouched and avoids installing MediaPipe's obsolete OpenCV 3.4.10.
if (Test-Path $OpenCvAdapterRoot) {
    Remove-Item -Recurse -Force $OpenCvAdapterRoot
}
New-Item -ItemType Directory -Force (Join-Path $OpenCvAdapterRoot 'include') | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OpenCvAdapterRoot 'lib') | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OpenCvAdapterRoot 'bin') | Out-Null
Copy-Item -Recurse -Force (Join-Path $OpenCvRoot 'include\opencv2') (Join-Path $OpenCvAdapterRoot 'include\opencv2')
Copy-Item -Force $OpenCvLib (Join-Path $OpenCvAdapterRoot 'lib\opencv_world480.lib')
Copy-Item -Force $OpenCvDll (Join-Path $OpenCvAdapterRoot 'bin\opencv_world480.dll')

$OpenCvBuild = @'
cc_import(
    name = "opencv_world_import",
    interface_library = "lib/opencv_world480.lib",
    shared_library = "bin/opencv_world480.dll",
)

cc_library(
    name = "opencv",
    hdrs = glob(["include/opencv2/**"]),
    includes = ["include"],
    deps = [":opencv_world_import"],
    visibility = ["//visibility:public"],
)
'@
Write-Utf8NoBom (Join-Path $OpenCvAdapterRoot 'BUILD.bazel') $OpenCvBuild
Write-Utf8NoBom (Join-Path $OpenCvAdapterRoot 'WORKSPACE') 'workspace(name = "windows_opencv")'

if (Test-Path $BridgeRoot) {
    Remove-Item -Recurse -Force $BridgeRoot
}
New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'api') | Out-Null
New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'src') | Out-Null
New-Item -ItemType Directory -Force (Join-Path $BridgeRoot 'tests') | Out-Null
Copy-Item (Join-Path $RepoRoot 'mediapipe\api\FaceMediaPipe.h') (Join-Path $BridgeRoot 'api\FaceMediaPipe.h')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\FaceMediaPipe.cpp') (Join-Path $BridgeRoot 'src\FaceMediaPipe.cpp')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\BgrToRgb.cpp') (Join-Path $BridgeRoot 'src\BgrToRgb.cpp')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\BgrToRgb.h') (Join-Path $BridgeRoot 'src\BgrToRgb.h')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\LandmarkConversion.cpp') (Join-Path $BridgeRoot 'src\LandmarkConversion.cpp')
Copy-Item (Join-Path $RepoRoot 'mediapipe\src\LandmarkConversion.h') (Join-Path $BridgeRoot 'src\LandmarkConversion.h')
Copy-Item (Join-Path $RepoRoot 'tests\bgr_to_rgb_test.cpp') (Join-Path $BridgeRoot 'tests\bgr_to_rgb_test.cpp')
Copy-Item (Join-Path $RepoRoot 'tests\landmark_conversion_test.cpp') (Join-Path $BridgeRoot 'tests\landmark_conversion_test.cpp')

$BridgeBuild = @'
cc_library(
    name = "FaceMediaPipe_impl",
    srcs = [
        "src/BgrToRgb.cpp",
        "src/LandmarkConversion.cpp",
        "src/FaceMediaPipe.cpp",
    ],
    hdrs = [
        "api/FaceMediaPipe.h",
        "src/BgrToRgb.h",
        "src/LandmarkConversion.h",
    ],
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
cc_test(
    name = "bgr_to_rgb_test",
    srcs = [
        "src/BgrToRgb.cpp",
        "src/BgrToRgb.h",
        "tests/bgr_to_rgb_test.cpp",
    ],
    includes = ["src"],
)
cc_test(
    name = "landmark_conversion_test",
    srcs = [
        "api/FaceMediaPipe.h",
        "src/LandmarkConversion.cpp",
        "src/LandmarkConversion.h",
        "tests/landmark_conversion_test.cpp",
    ],
    includes = [
        "api",
        "src",
    ],
)
'@
Write-Utf8NoBom (Join-Path $BridgeRoot 'BUILD.bazel') $BridgeBuild

# TensorFlow/FlatBuffers still contains shell-backed genrules on Windows.
# Never let Bazel pick C:\Windows\System32\bash.exe (the WSL launcher): that
# shell cannot directly execute the native Windows flatc.exe produced by Bazel.
# Prefer MSYS2 as recommended by Bazel; Git-for-Windows Bash is an acceptable
# fallback for these simple genrules. BAZEL_SH can override auto-detection.
$BazelSh = $env:BAZEL_SH
if (-not $BazelSh -or -not (Test-Path $BazelSh) -or $BazelSh -ieq "$env:WINDIR\System32\bash.exe") {
    $BashCandidates = @(
        'C:\msys64\usr\bin\bash.exe',
        'C:\Program Files\Git\usr\bin\bash.exe',
        'C:\Program Files\Git\bin\bash.exe'
    )
    $BazelSh = $null
    foreach ($candidate in $BashCandidates) {
        if (Test-Path $candidate) {
            $BazelSh = $candidate
            break
        }
    }
}
if (-not $BazelSh) {
    throw 'A native Windows Bash is required for TensorFlow/FlatBuffers genrules. Install MSYS2 x64 (recommended) or Git for Windows, or set BAZEL_SH to its bash.exe. Do not use C:\Windows\System32\bash.exe.'
}
$env:BAZEL_SH = (Resolve-Path $BazelSh).Path
Write-Host "Using Bazel Bash: $env:BAZEL_SH"

$BazelArgs = @('build')
$BazelArgs += '--compilation_mode=opt'
Write-Host 'Using Bazel compilation mode: opt'

# pthreadpool's BUILD passes GCC-style -std=c11. MSVC ignores that option.
# MSVC requires both /std:c11 and /experimental:c11atomics to enable
# <stdatomic.h> support used by pthreadpool.
$BazelArgs += '--conlyopt=/std:c11'
$BazelArgs += '--conlyopt=/experimental:c11atomics'
Write-Host 'Using MSVC C11 mode and C11 atomics for C dependencies'

# MediaPipe's MP_ASSIGN_OR_RETURN / status macros rely on conforming variadic
# macro expansion. MSVC's traditional preprocessor can leave helper tokens such
# as MP_STATUS_MACROS_IMPL_REM unexpanded. Enable Microsoft's conforming
# token-based preprocessor for all C++ compilation actions.
$BazelArgs += '--cxxopt=/Zc:preprocessor'
Write-Host 'Using MSVC conforming C++ preprocessor'

# Protobuf 5.28.3 declares JSON headers with strip_include_prefix="/src".
# On this Bazel/MSVC combination, exec/tool compilations can fail to resolve the
# generated virtual include for headers in the same target. Add the repository
# source root explicitly for both target and host/tool C++ actions.
$ProtoSrcInclude = 'external/com_google_protobuf/src'
$BazelArgs += "--cxxopt=/I$ProtoSrcInclude"
$BazelArgs += "--host_cxxopt=/I$ProtoSrcInclude"
Write-Host 'Adding direct Protobuf source include path for Windows tool builds'

$BazelArgs += '--repo_env=HERMETIC_PYTHON_VERSION=3.12'
Write-Host 'Using hermetic Python 3.12 for MediaPipe/TensorFlow repositories'
if ($env:TEMP -and (Test-Path $env:TEMP)) {
    Write-Host "Using Bazel distdir: $env:TEMP"
    $BazelArgs += "--distdir=$env:TEMP"
}
$OpenCvAdapterBazelPath = $OpenCvAdapterRoot -replace '\\','/'
$BazelArgs += "--override_repository=windows_opencv=$OpenCvAdapterBazelPath"
Write-Host "Overriding @windows_opencv with: $OpenCvAdapterRoot"
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
