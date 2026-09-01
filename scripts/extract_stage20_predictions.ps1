param(
    [Parameter(Mandatory = $true)] [string] $TraceDirectory,
    [Parameter(Mandatory = $true)] [string] $OutputCsv
)

$ErrorActionPreference = 'Stop'

function Add-StateRuns {
    param(
        [object[]] $Rows,
        [string] $ClipId,
        [string] $Event,
        [string] $Side,
        [scriptblock] $Predicate
    )

    $runs = @()
    $start = $null
    for ($index = 0; $index -lt $Rows.Count; ++$index) {
        $active = & $Predicate $Rows[$index]
        if ($active -and $null -eq $start) { $start = $index }
        if (-not $active -and $null -ne $start) {
            $runs += [pscustomobject]@{
                clip_id = $ClipId
                event = $Event
                side = $Side
                start_ms = [math]::Round([double]$Rows[$start].timestamp_ms, 3)
                end_ms = [math]::Round([double]$Rows[$index].timestamp_ms, 3)
                confidence = 1
            }
            $start = $null
        }
    }
    if ($null -ne $start) {
        $endMs = [double]$Rows[-1].timestamp_ms
        if ($Rows.Count -gt 1) {
            $endMs += [double]$Rows[-1].timestamp_ms - [double]$Rows[-2].timestamp_ms
        }
        $runs += [pscustomobject]@{
            clip_id = $ClipId
            event = $Event
            side = $Side
            start_ms = [math]::Round([double]$Rows[$start].timestamp_ms, 3)
            end_ms = [math]::Round($endMs, 3)
            confidence = 1
        }
    }
    return $runs
}

function Add-EyeClosureEvents {
    param([object[]] $Rows, [string] $ClipId)

    $events = @()
    $start = $null
    for ($index = 0; $index -le $Rows.Count; ++$index) {
        $closed = $index -lt $Rows.Count -and $Rows[$index].temporal_eye_state -eq 'closed'
        if ($closed -and $null -eq $start) { $start = $index }
        if ($closed -or $null -eq $start) { continue }

        $end = [math]::Min($index, $Rows.Count - 1)
        $lookaheadEnd = [math]::Min($Rows.Count - 1, $index + 10)
        $window = @($Rows[$start..$lookaheadEnd])
        $event = if (@($window | Where-Object prolonged_closure_event -eq '1').Count) {
            'prolonged_closure'
        } elseif (@($window | Where-Object long_blink_event -eq '1').Count) {
            'long_blink'
        } elseif (@($window | Where-Object temporal_blink_event -eq '1').Count) {
            'blink'
        } else {
            $null
        }

        if ($event) {
            $events += [pscustomobject]@{
                clip_id = $ClipId
                event = $event
                side = 'both'
                start_ms = [math]::Round([double]$Rows[$start].timestamp_ms, 3)
                end_ms = [math]::Round([double]$Rows[$end].timestamp_ms, 3)
                confidence = 1
            }
        }
        $start = $null
    }
    return $events
}

$predictions = @()
$traceFiles = @(Get-ChildItem -LiteralPath $TraceDirectory -Filter 'C*.csv' | Sort-Object Name)
if (-not $traceFiles.Count) { throw "No C*.csv traces found in $TraceDirectory" }

foreach ($file in $traceFiles) {
    $clipId = [System.IO.Path]::GetFileNameWithoutExtension($file.Name)
    $rows = @(Import-Csv -LiteralPath $file.FullName)
    if (-not $rows.Count) { throw "Trace is empty: $($file.FullName)" }

    $required = @('timestamp_ms', 'temporal_eye_state', 'temporal_blink_event',
                  'long_blink_event', 'prolonged_closure', 'prolonged_closure_event',
                  'yawn_active', 'head_zone', 'gaze_zone', 'presence')
    foreach ($column in $required) {
        if ($rows[0].PSObject.Properties.Name -notcontains $column) {
            throw "Trace $($file.Name) is missing column: $column"
        }
    }

    $predictions += Add-StateRuns $rows $clipId 'face_present' '' { param($row) $row.presence -eq 'present' }
    $predictions += Add-EyeClosureEvents $rows $clipId
    $predictions += Add-StateRuns $rows $clipId 'yawn' '' { param($row) $row.yawn_active -eq '1' }

    foreach ($zone in @('left', 'right', 'up', 'down')) {
        $headEvent = "head_$zone"
        $gazeEvent = "gaze_$zone"
        $predictions += Add-StateRuns $rows $clipId $headEvent '' {
            param($row) $row.head_zone -eq $zone
        }.GetNewClosure()
        $predictions += Add-StateRuns $rows $clipId $gazeEvent '' {
            param($row) $row.gaze_zone -eq $zone
        }.GetNewClosure()
    }
}

$parent = Split-Path -Parent $OutputCsv
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$predictions |
    Sort-Object clip_id, @{Expression = {[double]$_.start_ms}}, event, side |
    Export-Csv -NoTypeInformation -LiteralPath $OutputCsv
