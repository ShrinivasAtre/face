$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Header = Join-Path $RepoRoot 'third_party\mediapipe\mediapipe\framework\api3\calculator_context.h'

if (-not (Test-Path $Header)) {
    throw "MediaPipe calculator_context.h not found at $Header. Run scripts\fetch_mediapipe.ps1 first."
}

function Write-Utf8NoBom([string]$Path, [string]$Content) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

$content = [System.IO.File]::ReadAllText($Header)
$replacements = @(
    @(
        'template <typename T, int&... DoNotSpecify, typename F>',
        'template <typename T, typename F>'
    ),
    @(
        "template <typename T, typename U, typename... Rest, int&... DoNotSpecify,`r`n          typename F>",
        'template <typename T, typename U, typename... Rest, typename F>'
    ),
    @(
        "template <typename T, typename U, typename... Rest, int&... DoNotSpecify,`n          typename F>",
        'template <typename T, typename U, typename... Rest, typename F>'
    )
)

$changed = $false
foreach ($pair in $replacements) {
    if ($content.Contains($pair[0])) {
        $content = $content.Replace($pair[0], $pair[1])
        $changed = $true
    }
}

if ($content.Contains('template <typename T, int&... DoNotSpecify, typename F>') -or
    $content.Contains('typename... Rest, int&... DoNotSpecify')) {
    throw 'MediaPipe API3 MSVC compatibility patch did not apply cleanly.'
}

Write-Utf8NoBom $Header $content
if ($changed) {
    Write-Host 'Applied MediaPipe API3 template compatibility patch for MSVC.'
} else {
    Write-Host 'MediaPipe API3 template compatibility patch already applied.'
}
