param(
    [Parameter(Mandatory=$true)] [string] $AnnotatorADirectory,
    [Parameter(Mandatory=$true)] [string] $AnnotatorBDirectory,
    [Parameter(Mandatory=$true)] [string] $AdjudicationCsv,
    [Parameter(Mandatory=$true)] [string] $TraceDirectory,
    [Parameter(Mandatory=$true)] [string] $OutputDirectory,
    [double] $PerclosWindowMs = 60000.0,
    [double] $MinimumTruthCoverage = 0.80
)

$ErrorActionPreference = 'Stop'
if ($PerclosWindowMs -le 0.0) { throw 'PerclosWindowMs must be positive' }
if ($MinimumTruthCoverage -lt 0.0 -or $MinimumTruthCoverage -gt 1.0) {
    throw 'MinimumTruthCoverage must be in [0, 1]'
}
$fields = @('visibility', 'eye_state', 'occluder', 'quality')
$allowed = @{
    visibility = @('visible', 'partial', 'occluded', 'invalid', 'uncertain')
    eye_state = @('open', 'closed', 'transition', 'unknown')
    occluder = @('none', 'hand', 'object', 'glasses', 'other', 'unknown')
    quality = @('accepted', 'uncertain', 'exclude')
}

function Key($row) { "$($row.clip_id)|$($row.frame)|$($row.side)" }

$adjudication = @(Import-Csv -LiteralPath $AdjudicationCsv)
$adjudicationByKey = @{}
foreach ($row in $adjudication) {
    $key = "$($row.batch_id)|$(Key $row)"
    if ($adjudicationByKey.ContainsKey($key)) { throw "Duplicate adjudication key: $key" }
    $adjudicationByKey[$key] = $row
}

$finalEyes = @()
$agreement = @()
$batchFiles = @(Get-ChildItem -LiteralPath $AnnotatorADirectory -Filter 'labels-*.csv' | Sort-Object Name)
if (-not $batchFiles.Count) { throw "No labels-*.csv files found in $AnnotatorADirectory" }

foreach ($file in $batchFiles) {
    $batchId = $file.BaseName.Substring('labels-'.Length)
    $bPath = Join-Path $AnnotatorBDirectory $file.Name
    if (-not (Test-Path -LiteralPath $bPath)) { throw "Missing annotator B batch: $bPath" }
    $aRows = @(Import-Csv -LiteralPath $file.FullName)
    $bRows = @(Import-Csv -LiteralPath $bPath)
    $bByKey = @{}
    foreach ($row in $bRows) {
        $key = Key $row
        if ($bByKey.ContainsKey($key)) { throw "Duplicate annotator B key in ${batchId}: $key" }
        $bByKey[$key] = $row
    }
    if ($aRows.Count -ne $bRows.Count) { throw "Annotator row-count mismatch in $batchId" }

    $matchCounts = @{}; foreach ($field in $fields) { $matchCounts[$field] = 0 }
    $fullMatches = 0
    foreach ($a in $aRows) {
        $key = Key $a
        if (-not $bByKey.ContainsKey($key)) { throw "Annotator B is missing ${batchId}: $key" }
        $b = $bByKey[$key]
        $result = [ordered]@{
            batch_id=$batchId; clip_id=$a.clip_id; frame=$a.frame
            timestamp_ms=$a.timestamp_ms; side=$a.side; relative_path=$a.relative_path
        }
        $same = $true
        foreach ($field in $fields) {
            $aValue = $a.$field; $bValue = $b.$field
            if ($aValue -eq $bValue) {
                $matchCounts[$field]++
                $result[$field] = $aValue
            } else {
                $same = $false
                $adjKey = "$batchId|$key"
                if (-not $adjudicationByKey.ContainsKey($adjKey)) {
                    throw "Missing adjudication for ${batchId}: $key ($field)"
                }
                $adj = $adjudicationByKey[$adjKey]
                if ($adj.("a_$field") -ne $aValue -or $adj.("b_$field") -ne $bValue -or
                    $adj.("disagree_$field") -ne '1') {
                    throw "Adjudication source mismatch for ${batchId}: $key ($field)"
                }
                $value = $adj.("adjudicated_$field")
                if ([string]::IsNullOrWhiteSpace($value)) {
                    throw "Empty adjudication for ${batchId}: $key ($field)"
                }
                $result[$field] = $value
            }
            if ($result[$field] -notin $allowed[$field]) {
                throw "Invalid final $field for ${batchId}: $key"
            }
        }
        if ($same) { $fullMatches++ }
        if (-not $same) {
            $adjKey = "$batchId|$key"
            $adj = $adjudicationByKey[$adjKey]
            foreach ($field in $fields) {
                $expected = [int]($a.$field -ne $b.$field)
                if ([int]$adj.("disagree_$field") -ne $expected) {
                    throw "Incorrect disagreement flag for ${batchId}: $key ($field)"
                }
            }
            $adjudicationByKey.Remove($adjKey)
        }
        if ($result.visibility -eq 'visible' -and $result.occluder -notin @('none','glasses')) {
            throw "Visible eye has incompatible occluder for ${batchId}: $key"
        }
        if ($result.visibility -in @('occluded','invalid','uncertain') -and $result.eye_state -ne 'unknown') {
            throw "Non-observable eye must have unknown state for ${batchId}: $key"
        }
        $finalEyes += [pscustomobject]$result
    }
    $agreement += [pscustomobject]@{
        batch_id=$batchId; pairs=$aRows.Count
        visibility_agreement=[math]::Round($matchCounts.visibility/$aRows.Count,6)
        eye_state_agreement=[math]::Round($matchCounts.eye_state/$aRows.Count,6)
        occluder_agreement=[math]::Round($matchCounts.occluder/$aRows.Count,6)
        quality_agreement=[math]::Round($matchCounts.quality/$aRows.Count,6)
        full_row_agreement=[math]::Round($fullMatches/$aRows.Count,6)
        adjudicated_rows=$aRows.Count-$fullMatches
    }
}
if ($adjudicationByKey.Count) {
    throw "Adjudication contains $($adjudicationByKey.Count) row(s) without a source disagreement"
}

$traceByClip = @{}
foreach ($file in @(Get-ChildItem -LiteralPath $TraceDirectory -Filter 'C*.csv')) {
    $clipId = $file.BaseName
    $byFrame = @{}
    foreach ($row in @(Import-Csv -LiteralPath $file.FullName)) { $byFrame[$row.frame] = $row }
    $traceByClip[$clipId] = $byFrame
}

$samples = @()
foreach ($group in @($finalEyes | Group-Object batch_id,clip_id,frame)) {
    $eyes = @($group.Group)
    if ($eyes.Count -ne 2 -or @($eyes.side | Sort-Object -Unique).Count -ne 2) {
        throw "Expected one left/right pair at $($group.Name)"
    }
    $first = $eyes[0]
    if (-not $traceByClip.ContainsKey($first.clip_id) -or
        -not $traceByClip[$first.clip_id].ContainsKey($first.frame)) {
        throw "Trace row not found for $($first.clip_id) frame $($first.frame)"
    }
    $trace = $traceByClip[$first.clip_id][$first.frame]
    $accepted = @($eyes | Where-Object quality -eq 'accepted').Count -eq 2
    $observable = @($eyes | Where-Object {
        $_.visibility -in @('visible','partial') -and $_.eye_state -in @('open','closed')
    }).Count -eq 2
    $truth = 'unknown'
    if ($accepted -and $observable) {
        $truth = if (@($eyes | Where-Object eye_state -eq 'closed').Count) { 'closed' } else { 'open' }
    }
    $predicted = if ($trace.temporal_eye_state -in @('open','closed')) {
        $trace.temporal_eye_state
    } else { 'unknown' }
    $samples += [pscustomobject]@{
        batch_id=$first.batch_id; clip_id=$first.clip_id; frame=$first.frame
        timestamp_ms=$first.timestamp_ms; truth_state=$truth; predicted_state=$predicted
        truth_evaluable=[int]($truth -ne 'unknown')
        prediction_known=[int]($predicted -ne 'unknown')
        correct=[int]($truth -ne 'unknown' -and $predicted -ne 'unknown' -and $truth -eq $predicted)
        trace_eye_openness=$trace.eye_openness; trace_eye_usability=$trace.eye_usability
        trace_detected=$trace.detected; trace_semantic_valid=$trace.semantic_valid
        trace_eye_quality_confidence=$trace.eye_quality_confidence
        trace_eye_visibility=$trace.eye_visibility; trace_eye_contrast=$trace.eye_contrast
        trace_yaw_degrees=$trace.yaw_degrees; trace_pitch_degrees=$trace.pitch_degrees
        trace_perclos=$trace.perclos; trace_perclos_coverage=$trace.perclos_coverage
    }
}

foreach ($clip in @($samples | Group-Object clip_id)) {
    $ordered = @($clip.Group | Sort-Object {[double]$_.timestamp_ms})
    for ($index = 0; $index -lt $ordered.Count; ++$index) {
        if ($ordered.Count -eq 1) { $duration = 0.0 }
        elseif ($index -lt $ordered.Count - 1) {
            $duration = [double]$ordered[$index + 1].timestamp_ms - [double]$ordered[$index].timestamp_ms
        } else {
            $duration = [double]$ordered[$index].timestamp_ms - [double]$ordered[$index - 1].timestamp_ms
        }
        if ($duration -lt 0.0) { throw "Non-monotonic dense timestamps in $($clip.Name)" }
        $ordered[$index] | Add-Member -NotePropertyName duration_ms -NotePropertyValue $duration
    }
}

function Summarize($rows, $batchId, $clipId) {
    $evaluable = @($rows | Where-Object truth_evaluable -eq 1)
    $known = @($evaluable | Where-Object prediction_known -eq 1)
    $correct = @($known | Where-Object correct -eq 1)
    $truthClosed = @($evaluable | Where-Object truth_state -eq 'closed')
    $predictedClosed = @($known | Where-Object predicted_state -eq 'closed')
    $totalMs = ($rows | Measure-Object duration_ms -Sum).Sum
    $evaluableMs = ($evaluable | Measure-Object duration_ms -Sum).Sum
    $knownMs = ($known | Measure-Object duration_ms -Sum).Sum
    $correctMs = ($correct | Measure-Object duration_ms -Sum).Sum
    $truthClosedMs = ($truthClosed | Measure-Object duration_ms -Sum).Sum
    $predictedClosedMs = ($predictedClosed | Measure-Object duration_ms -Sum).Sum
    [pscustomobject]@{
        batch_id=$batchId; clip_id=$clipId; sampled_pairs=$rows.Count
        truth_evaluable=$evaluable.Count
        truth_known_coverage=if($rows.Count){[math]::Round($evaluable.Count/$rows.Count,6)}else{0}
        prediction_known=$known.Count
        prediction_known_coverage=if($evaluable.Count){[math]::Round($known.Count/$evaluable.Count,6)}else{0}
        correct_known=$correct.Count
        conditional_accuracy=if($known.Count){[math]::Round($correct.Count/$known.Count,6)}else{0}
        end_to_end_accuracy=if($evaluable.Count){[math]::Round($correct.Count/$evaluable.Count,6)}else{0}
        truth_closed_fraction=if($evaluable.Count){[math]::Round($truthClosed.Count/$evaluable.Count,6)}else{0}
        predicted_closed_fraction_known=if($known.Count){[math]::Round($predictedClosed.Count/$known.Count,6)}else{0}
        sampled_duration_ms=[math]::Round($totalMs,3)
        truth_evaluable_duration_ms=[math]::Round($evaluableMs,3)
        duration_truth_coverage=if($totalMs){[math]::Round($evaluableMs/$totalMs,6)}else{0}
        prediction_known_duration_ms=[math]::Round($knownMs,3)
        duration_prediction_known_coverage=if($evaluableMs){[math]::Round($knownMs/$evaluableMs,6)}else{0}
        duration_conditional_accuracy=if($knownMs){[math]::Round($correctMs/$knownMs,6)}else{0}
        duration_end_to_end_accuracy=if($evaluableMs){[math]::Round($correctMs/$evaluableMs,6)}else{0}
        duration_truth_closed_fraction=if($evaluableMs){[math]::Round($truthClosedMs/$evaluableMs,6)}else{0}
        duration_predicted_closed_fraction_known=if($knownMs){[math]::Round($predictedClosedMs/$knownMs,6)}else{0}
    }
}

$summary = @()
foreach ($group in @($samples | Group-Object batch_id,clip_id)) {
    $parts = $group.Name -split ', ',2
    $summary += Summarize @($group.Group) $parts[0] $parts[1]
}
$summary += Summarize @($samples) 'ALL' 'ALL'

$perclosWindows = @()
foreach ($clip in @($samples | Group-Object clip_id)) {
    $ordered = @($clip.Group | Sort-Object {[double]$_.timestamp_ms})
    $clipStart = [double]$ordered[0].timestamp_ms
    foreach ($endpoint in $ordered) {
        $endMs = [double]$endpoint.timestamp_ms
        $startMs = $endMs - $PerclosWindowMs
        if ($startMs -lt $clipStart) { continue }
        $knownMs = 0.0; $closedMs = 0.0
        foreach ($sample in $ordered) {
            $sampleStart = [double]$sample.timestamp_ms
            $sampleEnd = $sampleStart + [double]$sample.duration_ms
            $overlap = [math]::Min($sampleEnd, $endMs) - [math]::Max($sampleStart, $startMs)
            if ($overlap -le 0.0 -or $sample.truth_evaluable -ne 1) { continue }
            $knownMs += $overlap
            if ($sample.truth_state -eq 'closed') { $closedMs += $overlap }
        }
        $coverage = $knownMs / $PerclosWindowMs
        $truthPerclos = if ($coverage -ge $MinimumTruthCoverage -and $knownMs -gt 0.0) {
            $closedMs / $knownMs
        } else { $null }
        $modelPerclos = $null
        if (-not [string]::IsNullOrWhiteSpace($endpoint.trace_perclos)) {
            $modelPerclos = [double]$endpoint.trace_perclos
        }
        $comparable = $null -ne $truthPerclos -and $null -ne $modelPerclos
        $perclosWindows += [pscustomobject]@{
            batch_id=$endpoint.batch_id; clip_id=$endpoint.clip_id; end_frame=$endpoint.frame
            start_ms=[math]::Round($startMs,3); end_ms=[math]::Round($endMs,3)
            truth_known_coverage=[math]::Round($coverage,6)
            truth_perclos=if($null-ne$truthPerclos){[math]::Round($truthPerclos,6)}else{''}
            model_perclos=if($null-ne$modelPerclos){[math]::Round($modelPerclos,6)}else{''}
            model_perclos_coverage=$endpoint.trace_perclos_coverage
            comparable=[int]$comparable
            absolute_error=if($comparable){[math]::Round([math]::Abs($truthPerclos-$modelPerclos),6)}else{''}
        }
    }
}

$perclosSummary = @()
foreach ($group in @($perclosWindows | Where-Object comparable -eq 1 | Group-Object batch_id,clip_id)) {
    $values = @($group.Group)
    $parts = $group.Name -split ', ',2
    $perclosSummary += [pscustomobject]@{
        batch_id=$parts[0]; clip_id=$parts[1]; comparable_windows=$values.Count
        mean_absolute_error=[math]::Round(($values.absolute_error | Measure-Object -Average).Average,6)
        maximum_absolute_error=[math]::Round(($values.absolute_error | Measure-Object -Maximum).Maximum,6)
        mean_truth_perclos=[math]::Round(($values.truth_perclos | Measure-Object -Average).Average,6)
        mean_model_perclos=[math]::Round(($values.model_perclos | Measure-Object -Average).Average,6)
    }
}

$confusion = @()
foreach ($group in @($samples | Where-Object truth_evaluable -eq 1 |
    Group-Object batch_id,truth_state,predicted_state)) {
    $values=@($group.Group);$parts=$group.Name -split ', ',3
    $confusion += [pscustomobject]@{
        batch_id=$parts[0];truth_state=$parts[1];predicted_state=$parts[2]
        samples=$values.Count
        duration_ms=[math]::Round(($values.duration_ms|Measure-Object -Sum).Sum,3)
    }
}
$availability = @()
$usabilityEpisodes=@{}
foreach($clip in @($samples|Where-Object truth_evaluable -eq 1|Group-Object clip_id)){
    $previous=''
    foreach($sample in @($clip.Group|Sort-Object{[double]$_.timestamp_ms})){
        if($sample.trace_eye_usability-ne$previous){
            $episodeKey="$($sample.batch_id)|$($sample.trace_eye_usability)"
            $usabilityEpisodes[$episodeKey]=1+$(if($usabilityEpisodes.ContainsKey($episodeKey)){$usabilityEpisodes[$episodeKey]}else{0})
            $previous=$sample.trace_eye_usability
        }
    }
}
function Average-Numeric($values) {
    $numeric=@($values|Where-Object{-not[string]::IsNullOrWhiteSpace($_)}|ForEach-Object{[double]$_})
    if(-not$numeric.Count){return ''}
    return [math]::Round(($numeric|Measure-Object -Average).Average,6)
}
function Maximum-AbsoluteNumeric($values) {
    $numeric=@($values|Where-Object{-not[string]::IsNullOrWhiteSpace($_)}|ForEach-Object{[math]::Abs([double]$_)})
    if(-not$numeric.Count){return ''}
    return [math]::Round(($numeric|Measure-Object -Maximum).Maximum,6)
}
foreach ($group in @($samples | Where-Object truth_evaluable -eq 1 |
    Group-Object batch_id,trace_eye_usability)) {
    $values=@($group.Group);$parts=$group.Name -split ', ',2
    $known=@($values|Where-Object prediction_known -eq 1)
    $duration=($values.duration_ms|Measure-Object -Sum).Sum
    $knownDuration=($known.duration_ms|Measure-Object -Sum).Sum
    $availability += [pscustomobject]@{
        batch_id=$parts[0];trace_eye_usability=$parts[1];samples=$values.Count
        episodes=$usabilityEpisodes["$($parts[0])|$($parts[1])"]
        model_known_samples=$known.Count
        model_known_fraction=[math]::Round($known.Count/$values.Count,6)
        duration_ms=[math]::Round($duration,3)
        model_known_duration_fraction=if($duration){[math]::Round($knownDuration/$duration,6)}else{0}
        mean_eye_openness=Average-Numeric $values.trace_eye_openness
        detected_fraction=[math]::Round(@($values|Where-Object trace_detected -eq '1').Count/$values.Count,6)
        semantic_valid_fraction=[math]::Round(@($values|Where-Object trace_semantic_valid -eq '1').Count/$values.Count,6)
        mean_quality_confidence=Average-Numeric $values.trace_eye_quality_confidence
        mean_visibility=Average-Numeric $values.trace_eye_visibility
        mean_contrast=Average-Numeric $values.trace_eye_contrast
        maximum_absolute_yaw=Maximum-AbsoluteNumeric $values.trace_yaw_degrees
        maximum_absolute_pitch=Maximum-AbsoluteNumeric $values.trace_pitch_degrees
    }
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$agreement | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-agreement.csv')
$samples | Sort-Object clip_id,{[int]$_.frame} | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-samples.csv')
$summary | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-state-summary.csv')
$perclosWindows | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-perclos-windows.csv')
$perclosSummary | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-perclos-summary.csv')
$confusion | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-confusion.csv')
$availability | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $OutputDirectory 'dense-eye-availability-summary.csv')
$summary | Format-Table -AutoSize
