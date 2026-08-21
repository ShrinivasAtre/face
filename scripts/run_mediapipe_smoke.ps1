param(
    [Parameter(Mandatory = $true)]
    [string]$ModelPath,

    [Parameter(Mandatory = $true)]
    [string]$ImagePath,

    [string]$RuntimeDir = ""
)

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDir = Join-Path $RepoRoot 'build'
$SmokeSource = Join-Path $RepoRoot 'tests\mediapipe_smoke.cpp'
$BridgeDir = Join-Path $RepoRoot 'third_party\mediapipe\bazel-bin\face_bridge'
$BridgeDll = Join-Path $BridgeDir 'FaceMediaPipe.dll'
$BridgeImportLib = Join-Path $BridgeDir 'FaceMediaPipe.dll.if.lib'
$SmokeOutputDir = $BuildDir
if ($RuntimeDir) {
    $RuntimeDir = (Resolve-Path $RuntimeDir).Path
    $RuntimeBridgeDll = Join-Path $RuntimeDir 'FaceMediaPipe.dll'
    $RuntimeOpenCvDll = Join-Path $RuntimeDir 'opencv_world480.dll'
    foreach ($required in @($RuntimeBridgeDll, $RuntimeOpenCvDll)) {
        if (-not (Test-Path $required -PathType Leaf)) {
            throw "Required packaged runtime file not found: $required"
        }
    }
    $SmokeOutputDir = Join-Path $BuildDir 'mediapipe_package_smoke'
}
$SmokeExe = Join-Path $SmokeOutputDir 'mediapipe_smoke.exe'

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
    throw 'OpenCV 4.8.0 install not found. Set OPENCV_ROOT to the OpenCV install directory.'
}
$OpenCvRoot = (Resolve-Path $OpenCvRoot).Path
$OpenCvInclude = Join-Path $OpenCvRoot 'include'
$OpenCvLibDir = Join-Path $OpenCvRoot 'lib'
$OpenCvDll = Join-Path $OpenCvRoot 'bin\opencv_world480.dll'

foreach ($required in @($SmokeSource, $BridgeDll, $BridgeImportLib, $OpenCvDll, $ModelPath, $ImagePath)) {
    if (-not (Test-Path $required)) {
        throw "Required smoke-test input not found: $required"
    }
}

# The bridge and OpenCV import libraries are x64. Require an x64 Visual Studio
# developer shell so cl.exe emits a matching test executable.
if ($env:VSCMD_ARG_TGT_ARCH -and $env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    throw "Visual Studio target architecture is '$env:VSCMD_ARG_TGT_ARCH'. Open an x64 Developer PowerShell (amd64) and retry."
}

$cl = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $cl) {
    throw 'cl.exe was not found. Run this script from an x64 Developer PowerShell for Visual Studio.'
}
Write-Host "Using compiler: $($cl.Source)"
Write-Host "Using OpenCV 4.8.0 from: $OpenCvRoot"

New-Item -ItemType Directory -Force $SmokeOutputDir | Out-Null

$CompileArgs = @(
    '/nologo',
    '/EHsc',
    '/std:c++17',
    "/I$($RepoRoot)\mediapipe\api",
    "/I$OpenCvInclude",
    $SmokeSource,
    '/link',
    "/LIBPATH:$BridgeDir",
    "/LIBPATH:$OpenCvLibDir",
    'FaceMediaPipe.dll.if.lib',
    'opencv_world480.lib',
    "/OUT:$SmokeExe"
)

& cl.exe @CompileArgs
if ($LASTEXITCODE -ne 0) {
    throw "Smoke-test compilation failed with exit code $LASTEXITCODE."
}

if ($RuntimeDir) {
    Copy-Item -Force $RuntimeBridgeDll (Join-Path $SmokeOutputDir 'FaceMediaPipe.dll')
    Copy-Item -Force $RuntimeOpenCvDll (Join-Path $SmokeOutputDir 'opencv_world480.dll')
    Write-Host "Using packaged runtime from: $RuntimeDir"
}
else {
    Copy-Item -Force $BridgeDll (Join-Path $SmokeOutputDir 'FaceMediaPipe.dll')
    Copy-Item -Force $OpenCvDll (Join-Path $SmokeOutputDir 'opencv_world480.dll')
}

Write-Host "Running MediaPipe smoke test..."
& $SmokeExe (Resolve-Path $ModelPath).Path (Resolve-Path $ImagePath).Path
if ($LASTEXITCODE -ne 0) {
    throw "MediaPipe smoke test failed with exit code $LASTEXITCODE."
}
