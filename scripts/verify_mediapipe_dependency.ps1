$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$VersionFile = Join-Path $RepoRoot 'mediapipe\MEDIAPIPE_VERSION'
$BazelVersionFile = Join-Path $RepoRoot '.bazelversion'
$MediaPipeRoot = Join-Path $RepoRoot 'third_party\mediapipe'
$ExpectedOrigin = 'https://github.com/google-ai-edge/mediapipe.git'

foreach ($required in @($VersionFile, $BazelVersionFile)) {
    if (-not (Test-Path $required -PathType Leaf)) {
        throw "Required dependency contract file not found: $required"
    }
}

$Version = ConvertFrom-StringData (Get-Content -Raw $VersionFile)
if ($Version.Keys.Count -ne 2 -or
    $Version.MEDIAPIPE_TAG -notmatch '^v\d+\.\d+\.\d+$' -or
    $Version.MEDIAPIPE_COMMIT -notmatch '^[0-9a-f]{40}$') {
    throw 'MEDIAPIPE_VERSION must contain exactly MEDIAPIPE_TAG and a full MEDIAPIPE_COMMIT.'
}

$BazelVersion = (Get-Content -Raw $BazelVersionFile).Trim()
if ($BazelVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw "Invalid .bazelversion: $BazelVersion"
}

if (-not (Get-Command bazelisk -ErrorAction SilentlyContinue)) {
    throw 'bazelisk was not found on PATH.'
}
$BazelOutput = (& cmd.exe /d /s /c 'bazelisk version 2>&1' | Out-String)
if ($LASTEXITCODE -ne 0 -or
    ($BazelOutput -split "`r?`n") -notcontains "Build label: $BazelVersion") {
    throw "Effective Bazel version does not match $BazelVersion.`n$BazelOutput"
}

if (-not (Test-Path (Join-Path $MediaPipeRoot '.git')) -or
    -not (Test-Path (Join-Path $MediaPipeRoot 'WORKSPACE') -PathType Leaf)) {
    throw "Verified MediaPipe workspace not found at $MediaPipeRoot. Run scripts\fetch_mediapipe.ps1."
}

$Origin = (git -C $MediaPipeRoot remote get-url origin).Trim()
if ($LASTEXITCODE -ne 0 -or $Origin -ne $ExpectedOrigin) {
    throw "MediaPipe origin mismatch. Expected $ExpectedOrigin, got $Origin."
}

$Head = (git -C $MediaPipeRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $Head -ne $Version.MEDIAPIPE_COMMIT) {
    throw "MediaPipe commit mismatch. Expected $($Version.MEDIAPIPE_COMMIT), got $Head."
}

$TagCommit = (git -C $MediaPipeRoot rev-parse "$($Version.MEDIAPIPE_TAG)^{commit}").Trim()
if ($LASTEXITCODE -ne 0 -or $TagCommit -ne $Version.MEDIAPIPE_COMMIT) {
    throw "MediaPipe tag $($Version.MEDIAPIPE_TAG) does not resolve to $($Version.MEDIAPIPE_COMMIT)."
}

Write-Host 'MediaPipe dependency verification PASSED.'
Write-Host "Bazel: $BazelVersion"
Write-Host "MediaPipe: $($Version.MEDIAPIPE_TAG) ($Head)"
Write-Host "Origin: $Origin"
