param(
    [Parameter(Mandatory = $true)] [string] $TraceDirectory,
    [Parameter(Mandatory = $true)] [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

function Add-Runs {
    param($Rows, [string] $ClipId, [string] $Signal, [scriptblock] $Predicate, [string] $RootCause, [string] $Confidence)
    $runs = @()
    $start = $null
    $last = $null
    foreach ($row in $Rows) {
        $active = & $Predicate $row
        if ($active -and $null -eq $start) { $start = $row }
        if (-not $active -and $null -ne $start) {
            $runs += [pscustomobject]@{
                clip_id=$ClipId; signal=$Signal; start_frame=[int]$start.frame; end_frame=[int]$last.frame
                start_ms=[math]::Round([double]$start.timestamp_ms,3); end_ms=[math]::Round([double]$last.timestamp_ms,3)
                duration_ms=[math]::Round(([double]$last.timestamp_ms-[double]$start.timestamp_ms),3)
                root_cause=$RootCause; confidence=$Confidence
            }
            $start = $null
        }
        $last = $row
    }
    if ($null -ne $start) {
        $runs += [pscustomobject]@{
            clip_id=$ClipId; signal=$Signal; start_frame=[int]$start.frame; end_frame=[int]$last.frame
            start_ms=[math]::Round([double]$start.timestamp_ms,3); end_ms=[math]::Round([double]$last.timestamp_ms,3)
            duration_ms=[math]::Round(([double]$last.timestamp_ms-[double]$start.timestamp_ms),3)
            root_cause=$RootCause; confidence=$Confidence
        }
    }
    return $runs
}

$allIntervals = @()
$summaries = @()
$mapping = @()
$files = Get-ChildItem -LiteralPath $TraceDirectory -Filter '*.csv' |
    Where-Object Name -ne 'anonymous_clip_aggregates.csv' | Sort-Object Name
$index = 0
foreach ($file in $files) {
    ++$index
    $clipId = 'C{0:D2}' -f $index
    $rows = @(Import-Csv -LiteralPath $file.FullName)
    if ($rows.Count -eq 0) { continue }
    $mapping += [pscustomobject]@{clip_id=$clipId; private_file=$file.Name}

    $sets = @(
        @{n='eye_unknown'; p={param($r) $r.temporal_eye_state -eq 'unknown'}; c='quality_gate'; q='measured'},
        @{n='eye_closed'; p={param($r) $r.temporal_eye_state -eq 'closed'}; c='threshold_or_fsm'; q='candidate'},
        @{n='prolonged_closure'; p={param($r) $r.prolonged_closure -eq '1'}; c='threshold_or_fsm'; q='candidate'},
        @{n='blink_event'; p={param($r) $r.temporal_blink_event -eq '1'}; c='fsm'; q='measured'},
        @{n='yawn_active'; p={param($r) $r.yawn_active -eq '1'}; c='threshold_or_fsm'; q='candidate'},
        @{n='yawn_event'; p={param($r) $r.yawn_event -eq '1'}; c='fsm'; q='measured'},
        @{n='head_non_neutral'; p={param($r) $r.head_zone -in @('left','right','up','down')}; c='calibration_or_threshold'; q='candidate'},
        @{n='head_event'; p={param($r) $r.head_event -eq '1'}; c='fsm'; q='measured'},
        @{n='distracted'; p={param($r) $r.distracted -eq '1'}; c='calibration_or_threshold'; q='candidate'},
        @{n='distraction_event'; p={param($r) $r.distraction_event -eq '1'}; c='fsm'; q='measured'},
        @{n='face_missing'; p={param($r) $r.detected -ne '1'}; c='provider'; q='measured'}
    )
    foreach ($set in $sets) {
        $allIntervals += Add-Runs $rows $clipId $set.n $set.p $set.c $set.q
    }
    $summaries += [pscustomobject]@{
        clip_id=$clipId; frames=$rows.Count
        unknown_frames=@($rows | Where-Object temporal_eye_state -eq 'unknown').Count
        closed_frames=@($rows | Where-Object temporal_eye_state -eq 'closed').Count
        blink_events=@($rows | Where-Object temporal_blink_event -eq '1').Count
        prolonged_frames=@($rows | Where-Object prolonged_closure -eq '1').Count
        yawn_events=@($rows | Where-Object yawn_event -eq '1').Count
        head_events=@($rows | Where-Object head_event -eq '1').Count
        distracted_frames=@($rows | Where-Object distracted -eq '1').Count
        distraction_events=@($rows | Where-Object distraction_event -eq '1').Count
        missing_face_frames=@($rows | Where-Object detected -ne '1').Count
    }
}

$allIntervals | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $OutputDirectory 'trace_intervals.csv')
$summaries | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $OutputDirectory 'clip_summary.csv')
$mapping | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $OutputDirectory 'private_clip_mapping.csv')

[pscustomobject]@{
    clips=$summaries.Count; intervals=$allIntervals.Count
    eye_unknown_runs=@($allIntervals | Where-Object signal -eq 'eye_unknown').Count
    eye_closed_runs=@($allIntervals | Where-Object signal -eq 'eye_closed').Count
    blink_events=@($allIntervals | Where-Object signal -eq 'blink_event').Count
    yawn_events=@($allIntervals | Where-Object signal -eq 'yawn_event').Count
    head_events=@($allIntervals | Where-Object signal -eq 'head_event').Count
    distraction_events=@($allIntervals | Where-Object signal -eq 'distraction_event').Count
    face_missing_runs=@($allIntervals | Where-Object signal -eq 'face_missing').Count
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'analysis_totals.json') -Encoding utf8
