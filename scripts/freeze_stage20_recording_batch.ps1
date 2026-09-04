param([Parameter(Mandatory=$true)] [string] $BatchRoot)

$ErrorActionPreference = 'Stop'
$inventoryPath = Join-Path $BatchRoot 'recording-inventory.csv'
if (-not (Test-Path -LiteralPath $inventoryPath)) { throw "Missing inventory: $inventoryPath" }
$rows = @(Import-Csv -LiteralPath $inventoryPath)
if (-not $rows.Count) { throw 'Recording inventory is empty' }
$seen = @{}; $frozen = @()
foreach ($row in $rows) {
    if ($seen.ContainsKey($row.clip_id)) { throw "Duplicate clip_id: $($row.clip_id)" }
    $seen[$row.clip_id] = $true
    if ($row.consent_confirmed -ne 'yes') { throw "Consent is not confirmed for $($row.clip_id)" }
    if ($row.recorded -ne 'yes') { throw "Recording is not marked complete for $($row.clip_id)" }
    if (-not (Test-Path -LiteralPath $row.private_source_path -PathType Leaf)) {
        throw "Missing recording for $($row.clip_id): $($row.private_source_path)"
    }
    $item = Get-Item -LiteralPath $row.private_source_path
    if ($item.Length -le 0) { throw "Recording is empty for $($row.clip_id)" }
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $item.FullName).Hash.ToLowerInvariant()
    $row.sha256 = $hash; $row.bytes = $item.Length
    $frozen += [pscustomobject]@{
        clip_id=$row.clip_id;subject_id=$row.subject_id;session_id=$row.session_id;slice=$row.slice
        sha256=$hash;requires_two_annotators='True';private_source_path=$item.FullName
    }
}
$rows | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath $inventoryPath
$mapping = Join-Path $BatchRoot 'frozen\private-clip-mapping-and-checksums.csv'
$frozen | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath $mapping
Write-Output "FROZEN $($frozen.Count) RECORDINGS; MAPPING=$mapping"
