$ErrorActionPreference = 'Stop'
$PackageRoot = $PSScriptRoot
$Manifest = Join-Path $PackageRoot 'APPLICATION_MANIFEST.txt'
if (-not (Test-Path $Manifest -PathType Leaf)) { throw "Manifest is missing: $Manifest" }
$lines = @(Get-Content $Manifest)
$start = [Array]::IndexOf($lines, 'payload_sha256:') + 1
$end = [Array]::IndexOf($lines, 'application_dependencies:')
if ($start -le 0 -or $end -le $start) { throw 'Manifest payload section is invalid.' }
$checked = 0
for ($index = $start; $index -lt $end; ++$index) {
    if ($lines[$index] -notmatch '^([0-9a-f]{64})  (.+)$') {
        throw "Invalid payload record: $($lines[$index])"
    }
    $path = Join-Path $PackageRoot $Matches[2]
    if (-not (Test-Path $path -PathType Leaf)) { throw "Payload is missing: $($Matches[2])" }
    $actual = (Get-FileHash $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Matches[1]) { throw "Payload checksum mismatch: $($Matches[2])" }
    ++$checked
}
Write-Host "PACKAGE VERIFY PASSED ($checked files)"
