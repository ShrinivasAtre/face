param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDir,

    [string]$Configuration = 'Release',
    [string]$OutputDir = '',

    [string]$VideoDir = ''
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$SourceRevision = (& git -c "safe.directory=$RepoRoot" -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or -not $SourceRevision) { throw 'Could not determine source revision.' }
$BuildDir = (Resolve-Path $BuildDir).Path
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot 'dist\application\windows-x64'
}
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$driveRoot = [System.IO.Path]::GetPathRoot($OutputDir)
if ($OutputDir -eq $driveRoot -or $OutputDir -eq $RepoRoot) {
    throw "Unsafe application package output directory: $OutputDir"
}

$AppDir = Join-Path $BuildDir $Configuration
$Inputs = [ordered]@{
    'yunet_demo.exe' = Join-Path $AppDir 'yunet_demo.exe'
    'face_benchmark.exe' = Join-Path $AppDir 'face_benchmark.exe'
    'dms_sponsor_selftest.exe' = Join-Path $AppDir 'dms_sponsor_selftest.exe'
    'FaceMediaPipe.dll' = Join-Path $AppDir 'FaceMediaPipe.dll'
    'opencv_world480.dll' = Join-Path $AppDir 'opencv_world480.dll'
    'FaceMediaPipe.MANIFEST.txt' = Join-Path $AppDir 'FaceMediaPipe.MANIFEST.txt'
    'models/face_detection_yunet_2026may.onnx' = Join-Path $AppDir 'models\face_detection_yunet_2026may.onnx'
    'models/lbfmodel.yaml' = Join-Path $AppDir 'models\lbfmodel.yaml'
    'models/mediapipe/face_landmarker.task' = Join-Path $AppDir 'models\mediapipe\face_landmarker.task'
    'run_face.ps1' = Join-Path $PSScriptRoot 'run_deployed_face.ps1'
    'run_self_test.ps1' = Join-Path $PSScriptRoot 'run_sponsor_selftest.ps1'
    'run_video_demo.ps1' = Join-Path $PSScriptRoot 'run_sponsor_video_demo.ps1'
    'verify_package.ps1' = Join-Path $PSScriptRoot 'verify_sponsor_package.ps1'
    'test-data/synthetic_eye_sequence.csv' = Join-Path $RepoRoot 'demo\synthetic_eye_sequence.csv'
    'VIDEO_CATALOG.md' = Join-Path $RepoRoot 'demo\SPONSOR_VIDEO_CATALOG.md'
    'README_SPONSOR_DEMO.md' = Join-Path $RepoRoot 'docs\SPONSOR_DEMO.md'
}
$RuntimeDlls = @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll', 'concrt140.dll')
foreach ($runtimeDll in $RuntimeDlls) {
    $runtimePath = Join-Path $env:SystemRoot "System32\$runtimeDll"
    if (-not (Test-Path $runtimePath -PathType Leaf)) {
        throw "Required application-local Visual C++ runtime is missing: $runtimePath"
    }
    $Inputs[$runtimeDll] = $runtimePath
}
foreach ($entry in $Inputs.GetEnumerator()) {
    if (-not (Test-Path $entry.Value -PathType Leaf)) {
        throw "Required application package input is missing: $($entry.Value)"
    }
}

function Find-Dumpbin {
    $command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($install) {
            $candidate = Get-ChildItem `
                (Join-Path $install 'VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe') `
                -ErrorAction SilentlyContinue |
                Sort-Object FullName -Descending | Select-Object -First 1
            if ($candidate) { return $candidate.FullName }
        }
    }
    throw 'dumpbin.exe not found. Install the Visual C++ x64 build tools.'
}

$Dumpbin = Find-Dumpbin
foreach ($binary in @($Inputs['yunet_demo.exe'], $Inputs['face_benchmark.exe'], $Inputs['FaceMediaPipe.dll'])) {
    $headers = (& $Dumpbin /headers $binary | Out-String)
    if ($LASTEXITCODE -ne 0 -or $headers -notmatch '8664 machine \(x64\)') {
        throw "Expected an x64 PE binary: $binary"
    }
}
$dependencyOutput = ((& $Dumpbin /dependents $Inputs['yunet_demo.exe'] | Out-String) +
    (& $Dumpbin /dependents $Inputs['face_benchmark.exe'] | Out-String))
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect application dependencies.' }
$dependencies = [regex]::Matches(
    $dependencyOutput, '(?im)^\s+([A-Za-z0-9_.-]+\.dll)\s*$') |
    ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } |
    Sort-Object -Unique
if ($dependencies -contains 'facemediapipe.dll') {
    throw 'The application links directly to FaceMediaPipe.dll.'
}

$modelHash = (Get-FileHash $Inputs['models/mediapipe/face_landmarker.task'] -Algorithm SHA256).Hash.ToLowerInvariant()
$expectedModelHash = '64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff'
if ($modelHash -ne $expectedModelHash) {
    throw "Face Landmarker model hash mismatch: $modelHash"
}
$bridgeManifest = Get-Content $Inputs['FaceMediaPipe.MANIFEST.txt'] -Raw
if ($bridgeManifest -notmatch '(?m)^platform=windows-x64\r?$' -or
    $bridgeManifest -notmatch '(?m)^model_bundled=false\r?$') {
    throw 'Bridge manifest does not match the Windows package boundary.'
}

if (Test-Path $OutputDir) { Remove-Item -LiteralPath $OutputDir -Recurse -Force }
New-Item -ItemType Directory -Force $OutputDir | Out-Null
foreach ($entry in $Inputs.GetEnumerator()) {
    $destination = Join-Path $OutputDir $entry.Key
    New-Item -ItemType Directory -Force (Split-Path $destination -Parent) | Out-Null
    Copy-Item -LiteralPath $entry.Value -Destination $destination
}
if ($VideoDir) {
    $VideoDir = (Resolve-Path $VideoDir).Path
    $videoFiles = @(Get-ChildItem $VideoDir -Recurse -File |
        Where-Object { $_.Extension -match '^\.(mp4|avi|mov|mkv)$' })
    if ($videoFiles.Count -eq 0) { throw "No supported video files found in: $VideoDir" }
    foreach ($video in $videoFiles) {
        $relative = $video.FullName.Substring($VideoDir.TrimEnd('\').Length + 1)
        $destination = Join-Path $OutputDir (Join-Path 'videos' $relative)
        New-Item -ItemType Directory -Force (Split-Path $destination -Parent) | Out-Null
        Copy-Item -LiteralPath $video.FullName -Destination $destination
    }
}

$hashLines = Get-ChildItem $OutputDir -Recurse -File |
    Sort-Object FullName |
    ForEach-Object {
        # Windows PowerShell 5.1 does not provide Path.GetRelativePath().
        # Every enumerated file is below OutputDir, so derive a stable path
        # directly from the validated output root.
        $relative = $_.FullName.Substring($OutputDir.TrimEnd('\').Length + 1).Replace('\', '/')
        $hash = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $relative"
    }
$manifestLines = @(
    'face-application-package-v2-sponsor-demo'
    'platform=windows-x64'
    'architecture=x64'
    "source_revision=$SourceRevision"
    'backends=yunet,mediapipe'
    'mediapipe_model=models/mediapipe/face_landmarker.task'
    'payload_sha256:'
) + $hashLines + @(
    'application_dependencies:'
) + $dependencies + @(
    'launch:'
    'powershell -ExecutionPolicy Bypass -File .\run_face.ps1 yunet'
    'powershell -ExecutionPolicy Bypass -File .\run_face.ps1 mediapipe'
    'powershell -ExecutionPolicy Bypass -File .\run_self_test.ps1'
    'powershell -ExecutionPolicy Bypass -File .\run_video_demo.ps1 mediapipe'
    'powershell -ExecutionPolicy Bypass -File .\verify_package.ps1'
    'platform_prerequisites:'
    'Windows x64 system DLLs'
    'Windows Media Foundation codecs for the selected video format'
)
[System.IO.File]::WriteAllText(
    (Join-Path $OutputDir 'APPLICATION_MANIFEST.txt'),
    (($manifestLines -join "`n") + "`n"),
    [System.Text.UTF8Encoding]::new($false))

Write-Host 'Final Windows application package completed.'
Write-Host "Output: $OutputDir"
