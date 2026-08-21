$ErrorActionPreference = 'Stop'

$RootDir = Split-Path -Parent $PSScriptRoot
$MpDir = Join-Path $RootDir 'third_party\mediapipe'
$VersionFile = Join-Path $RootDir 'mediapipe\MEDIAPIPE_VERSION'
$VerifyScript = Join-Path $PSScriptRoot 'verify_mediapipe_dependency.ps1'

if (-not (Test-Path $VersionFile -PathType Leaf)) {
    throw "MediaPipe version file not found: $VersionFile"
}
$Version = ConvertFrom-StringData (Get-Content -Raw $VersionFile)
$ExpectedTag = $Version.MEDIAPIPE_TAG
$ExpectedCommit = $Version.MEDIAPIPE_COMMIT
if ($Version.Keys.Count -ne 2 -or
    $ExpectedTag -notmatch '^v\d+\.\d+\.\d+$' -or
    $ExpectedCommit -notmatch '^[0-9a-f]{40}$') {
    throw 'Invalid mediapipe/MEDIAPIPE_VERSION contract.'
}

if (-not (Test-Path (Join-Path $MpDir '.git'))) {
    New-Item -ItemType Directory -Force -Path (Join-Path $RootDir 'third_party') | Out-Null
    git clone --filter=blob:none --no-checkout https://github.com/google-ai-edge/mediapipe.git $MpDir
}

git -C $MpDir fetch --tags --depth=1 origin $ExpectedTag
git -C $MpDir checkout --detach $ExpectedCommit

$ActualCommit = (git -C $MpDir rev-parse HEAD).Trim()
if ($ActualCommit -ne $ExpectedCommit) {
    throw "MediaPipe commit mismatch. Expected $ExpectedCommit, got $ActualCommit."
}

& $VerifyScript
Write-Host "MediaPipe $ExpectedTag ($ActualCommit) is ready in $MpDir"
