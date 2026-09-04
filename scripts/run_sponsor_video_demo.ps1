param(
    [ValidateSet('yunet', 'mediapipe')]
    [string]$Backend = 'mediapipe',
    [string]$Video = ''
)

$ErrorActionPreference = 'Stop'
$PackageRoot = $PSScriptRoot
$Executable = Join-Path $PackageRoot 'face_benchmark.exe'
if (-not (Test-Path $Executable -PathType Leaf)) {
    throw "Demo executable is missing: $Executable"
}

if (-not $Video) {
    $videos = @(Get-ChildItem (Join-Path $PackageRoot 'videos') -Recurse -File |
        Where-Object { $_.Extension -match '^\.(mp4|avi|mov|mkv)$' } |
        Sort-Object FullName)
    if ($videos.Count -eq 0) { throw 'No demonstration videos are installed.' }
    Write-Host ''
    Write-Host 'Available demonstration videos:'
    for ($index = 0; $index -lt $videos.Count; ++$index) {
        Write-Host ("  [{0}] {1}" -f ($index + 1), $videos[$index].BaseName)
    }
    $selection = 0
    if (-not [int]::TryParse((Read-Host 'Select video number'), [ref]$selection) -or
        $selection -lt 1 -or $selection -gt $videos.Count) {
        throw 'Invalid video selection.'
    }
    $Video = $videos[$selection - 1].FullName
} elseif (-not [System.IO.Path]::IsPathRooted($Video)) {
    $Video = Join-Path $PackageRoot $Video
}
if (-not (Test-Path $Video -PathType Leaf)) { throw "Video is missing: $Video" }

$ResultDirectory = Join-Path $PackageRoot 'results'
New-Item -ItemType Directory -Force $ResultDirectory | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$result = Join-Path $ResultDirectory ("demo-{0}-{1}.json" -f $Backend, $stamp)

Write-Host "Starting DMS engineering demo"
Write-Host "Backend: $Backend"
Write-Host "Video:   $Video"
Write-Host 'Press Q or Esc to stop playback.'
& $Executable "--backend=$Backend" "--input=$Video" --warmup=10 `
    "--output=$result" --sponsor-demo
if ($LASTEXITCODE -ne 0) { throw "Demo failed with exit code $LASTEXITCODE." }
Write-Host "Result: $result"
