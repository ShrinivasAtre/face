$ErrorActionPreference = 'Stop'

$RootDir = Split-Path -Parent $PSScriptRoot
$MpDir = Join-Path $RootDir 'third_party\mediapipe'
$ExpectedTag = 'v0.10.33'
$ExpectedCommit = '3987048'

if (-not (Test-Path (Join-Path $MpDir '.git'))) {
    New-Item -ItemType Directory -Force -Path (Join-Path $RootDir 'third_party') | Out-Null
    git clone --filter=blob:none --no-checkout https://github.com/google-ai-edge/mediapipe.git $MpDir
}

git -C $MpDir fetch --tags --depth=1 origin $ExpectedTag
git -C $MpDir checkout --detach $ExpectedTag

$ActualCommit = (git -C $MpDir rev-parse --short=7 HEAD).Trim()
if ($ActualCommit -ne $ExpectedCommit) {
    throw "MediaPipe commit mismatch. Expected $ExpectedCommit, got $ActualCommit."
}

Write-Host "MediaPipe $ExpectedTag ($ActualCommit) is ready in $MpDir"
