param(
    [Parameter(Mandatory = $true)] [string] $Scores,
    [Parameter(Mandatory = $true)] [string] $OutputCsv
)

$ErrorActionPreference = 'Stop'
$rows = @(Import-Csv -LiteralPath $Scores)
if (-not $rows.Count) { throw "Score file has no rows: $Scores" }

foreach ($column in @('event', 'truth_count', 'predicted_count', 'tp', 'fp', 'fn')) {
    if ($rows[0].PSObject.Properties.Name -notcontains $column) {
        throw "Score file is missing column: $column"
    }
}

$summary = foreach ($group in $rows | Group-Object event | Sort-Object Name) {
    $truth = [int](($group.Group.truth_count | Measure-Object -Sum).Sum)
    $predicted = [int](($group.Group.predicted_count | Measure-Object -Sum).Sum)
    $tp = [int](($group.Group.tp | Measure-Object -Sum).Sum)
    $fp = [int](($group.Group.fp | Measure-Object -Sum).Sum)
    $fn = [int](($group.Group.fn | Measure-Object -Sum).Sum)
    $precision = if ($tp + $fp) { $tp / ($tp + $fp) } else { 0.0 }
    $recall = if ($tp + $fn) { $tp / ($tp + $fn) } else { 0.0 }
    $f1 = if ($precision + $recall) { 2 * $precision * $recall / ($precision + $recall) } else { 0.0 }
    [pscustomobject]@{
        event = $group.Name
        truth = $truth
        predicted = $predicted
        tp = $tp
        fp = $fp
        fn = $fn
        precision = [math]::Round($precision, 3)
        recall = [math]::Round($recall, 3)
        f1 = [math]::Round($f1, 3)
    }
}

$parent = Split-Path -Parent $OutputCsv
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$summary | Export-Csv -NoTypeInformation -LiteralPath $OutputCsv

