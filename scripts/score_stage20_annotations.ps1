param(
    [Parameter(Mandatory=$true)] [string] $GroundTruth,
    [Parameter(Mandatory=$true)] [string] $Predictions,
    [Parameter(Mandatory=$true)] [string] $OutputCsv,
    [double] $DefaultToleranceMs = 100.0
)
$ErrorActionPreference = 'Stop'
$truth = @(Import-Csv -LiteralPath $GroundTruth | Where-Object label_quality -ne 'uncertain')
$pred = @(Import-Csv -LiteralPath $Predictions)
foreach ($column in @('clip_id','event','side','start_ms','end_ms','label_quality')) {
    if ($truth.Count -and $truth[0].PSObject.Properties.Name -notcontains $column) { throw "Ground truth missing column: $column" }
}
foreach ($column in @('clip_id','event','side','start_ms','end_ms','confidence')) {
    if ($pred.Count -and $pred[0].PSObject.Properties.Name -notcontains $column) { throw "Predictions missing column: $column" }
}
function Get-Percentile([double[]]$Values, [double]$P) {
    if (-not $Values.Count) { return '' }
    $v=@($Values|Sort-Object); $i=[math]::Ceiling($P*$v.Count)-1
    return [math]::Round($v[[math]::Max(0,$i)],3)
}
$result = @()
$keys = @($truth + $pred | ForEach-Object { "$($_.clip_id)|$($_.event)|$($_.side)" } | Sort-Object -Unique)
foreach ($key in $keys) {
    $parts = $key -split '\|', 3
    $t = @($truth | Where-Object { $_.clip_id -eq $parts[0] -and $_.event -eq $parts[1] -and $_.side -eq $parts[2] })
    $p = @($pred | Where-Object { $_.clip_id -eq $parts[0] -and $_.event -eq $parts[1] -and $_.side -eq $parts[2] })
    $used = @{}
    $delays = @(); $offsetDelays=@()
    $tp = 0
    foreach ($item in $t) {
        $tol = if ($item.boundary_tolerance_ms -ne '') { [double]$item.boundary_tolerance_ms } else { $DefaultToleranceMs }
        $best = $null; $bestDistance = [double]::PositiveInfinity
        for ($i=0; $i -lt $p.Count; ++$i) {
            if ($used.ContainsKey($i)) { continue }
            $overlap = [math]::Min([double]$item.end_ms,[double]$p[$i].end_ms) - [math]::Max([double]$item.start_ms,[double]$p[$i].start_ms)
            $distance = [math]::Abs([double]$p[$i].start_ms-[double]$item.start_ms)
            if (($overlap -gt 0 -or $distance -le $tol) -and $distance -lt $bestDistance) { $best=$i; $bestDistance=$distance }
        }
        if ($null -ne $best) {
            $used[$best]=$true; ++$tp
            $delays += ([double]$p[$best].start_ms-[double]$item.start_ms)
            $offsetDelays += ([double]$p[$best].end_ms-[double]$item.end_ms)
        }
    }
    $fp = $p.Count-$tp; $fn = $t.Count-$tp
    $precision = if ($tp+$fp -gt 0) { $tp/($tp+$fp) } else { 0.0 }
    $recall = if ($tp+$fn -gt 0) { $tp/($tp+$fn) } else { 0.0 }
    $f1 = if ($precision+$recall -gt 0) { 2*$precision*$recall/($precision+$recall) } else { 0.0 }
    $result += [pscustomobject]@{
        clip_id=$parts[0]; event=$parts[1]; side=$parts[2]; truth_count=$t.Count; predicted_count=$p.Count
        tp=$tp; fp=$fp; fn=$fn; precision=[math]::Round($precision,6); recall=[math]::Round($recall,6); f1=[math]::Round($f1,6)
        signed_count_error=$p.Count-$t.Count; absolute_count_error=[math]::Abs($p.Count-$t.Count)
        mean_onset_delay_ms=if($delays.Count){[math]::Round(($delays|Measure-Object -Average).Average,3)}else{''}
        p50_absolute_onset_delay_ms=Get-Percentile @($delays|ForEach-Object {[math]::Abs($_)}) 0.50
        p95_absolute_onset_delay_ms=Get-Percentile @($delays|ForEach-Object {[math]::Abs($_)}) 0.95
        mean_offset_delay_ms=if($offsetDelays.Count){[math]::Round(($offsetDelays|Measure-Object -Average).Average,3)}else{''}
    }
}
$result | Export-Csv -NoTypeInformation -LiteralPath $OutputCsv
