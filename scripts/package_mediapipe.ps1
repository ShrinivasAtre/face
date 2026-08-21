param(
    [string]$OutputDir = ""
)

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BridgeDir = Join-Path $RepoRoot 'third_party\mediapipe\bazel-bin\face_bridge'
$BridgeDll = Join-Path $BridgeDir 'FaceMediaPipe.dll'

if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot 'dist\mediapipe\windows-x64'
}
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)

$OpenCvRoot = $env:OPENCV_ROOT
if (-not $OpenCvRoot) {
    foreach ($candidate in @('C:\opencv-4.8.0-src\install', 'C:\opencv\build')) {
        if (Test-Path (Join-Path $candidate 'bin\opencv_world480.dll')) {
            $OpenCvRoot = $candidate
            break
        }
    }
}
if (-not $OpenCvRoot) {
    throw 'OpenCV 4.8.0 install not found. Set OPENCV_ROOT.'
}
$OpenCvDll = Join-Path $OpenCvRoot 'bin\opencv_world480.dll'

foreach ($required in @($BridgeDll, $OpenCvDll)) {
    if (-not (Test-Path $required -PathType Leaf)) {
        throw "Required package input not found: $required"
    }
}

function Find-Dumpbin {
    $command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($install) {
            $candidate = Get-ChildItem `
                (Join-Path $install 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') `
                -ErrorAction SilentlyContinue |
                Sort-Object FullName -Descending |
                Select-Object -First 1
            if ($candidate) {
                return $candidate.FullName
            }
        }
    }

    throw 'dumpbin.exe not found. Install the Visual C++ x64 build tools.'
}

$Dumpbin = Find-Dumpbin
$Headers = (& $Dumpbin /headers $BridgeDll | Out-String)
if ($LASTEXITCODE -ne 0 -or $Headers -notmatch '8664 machine \(x64\)') {
    throw 'FaceMediaPipe.dll is not an x64 DLL.'
}

$DependencyOutput = (& $Dumpbin /dependents $BridgeDll | Out-String)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not inspect FaceMediaPipe.dll dependencies.'
}
$Dependencies = [regex]::Matches(
    $DependencyOutput,
    '(?im)^\s+([A-Za-z0-9_.-]+\.dll)\s*$') |
    ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } |
    Sort-Object -Unique

$Forbidden = $Dependencies | Where-Object {
    $_ -match 'mediapipe|tensorflow|tflite|protobuf|absl'
}
if ($Forbidden) {
    throw "Forbidden bridge runtime dependency: $($Forbidden -join ', ')"
}
if ($Dependencies -notcontains 'opencv_world480.dll') {
    throw 'FaceMediaPipe.dll does not depend on the packaged opencv_world480.dll.'
}

$ExportOutput = (& $Dumpbin /exports $BridgeDll | Out-String)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not inspect FaceMediaPipe.dll exports.'
}
$Exports = [regex]::Matches($ExportOutput, '\bface_mp_[A-Za-z0-9_]+\b') |
    ForEach-Object { $_.Value } |
    Sort-Object -Unique
$ExpectedExports = @(
    'face_mp_api_version',
    'face_mp_create',
    'face_mp_destroy',
    'face_mp_last_error',
    'face_mp_process_bgr'
)
if (($Exports -join "`n") -ne ($ExpectedExports -join "`n")) {
    throw "Unexpected face_mp exports: $($Exports -join ', ')"
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null
$AllowedNames = @('FaceMediaPipe.dll', 'opencv_world480.dll', 'MANIFEST.txt')
$Unexpected = Get-ChildItem -Force $OutputDir | Where-Object {
    $_.Name -notin $AllowedNames
}
if ($Unexpected) {
    throw "Refusing to clean output containing unexpected entries: $($Unexpected.Name -join ', ')"
}
foreach ($name in $AllowedNames) {
    $path = Join-Path $OutputDir $name
    if (Test-Path $path) {
        Remove-Item -Force -LiteralPath $path
    }
}

Copy-Item -LiteralPath $BridgeDll -Destination (Join-Path $OutputDir 'FaceMediaPipe.dll')
Copy-Item -LiteralPath $OpenCvDll -Destination (Join-Path $OutputDir 'opencv_world480.dll')

$Hashes = Get-ChildItem $OutputDir -File |
    Where-Object { $_.Name -ne 'MANIFEST.txt' } |
    Sort-Object Name |
    ForEach-Object {
        $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash.ToLowerInvariant()
        "$hash  $($_.Name)"
    }

$Lines = @(
    'face-mediapipe-package-v1',
    'platform=windows-x64',
    'model_bundled=false',
    'payload_sha256:'
) + $Hashes + @(
    'direct_dependencies:'
) + ($Dependencies | ForEach-Object { "$_" }) + @(
    'exports:'
) + $Exports + @(
    'platform_prerequisites:',
    'Windows x64 system DLLs',
    'Microsoft Visual C++ runtime compatible with the build toolchain'
)

[System.IO.File]::WriteAllText(
    (Join-Path $OutputDir 'MANIFEST.txt'),
    (($Lines -join "`n") + "`n"),
    [System.Text.UTF8Encoding]::new($false))

Write-Host "MediaPipe Windows package created: $OutputDir"
Get-Content (Join-Path $OutputDir 'MANIFEST.txt')
