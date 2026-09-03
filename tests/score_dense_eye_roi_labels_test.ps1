$ErrorActionPreference = 'Stop'
$root = Join-Path ([System.IO.Path]::GetTempPath()) ("dense-eye-score-" + [guid]::NewGuid())
try {
    $aDir = New-Item -ItemType Directory -Path (Join-Path $root 'a')
    $bDir = New-Item -ItemType Directory -Path (Join-Path $root 'b')
    $traces = New-Item -ItemType Directory -Path (Join-Path $root 'traces')
    $output = Join-Path $root 'output'

    function Eye($frame, $side, $state, $annotator) {
        [pscustomobject]@{
            clip_id='C01'; frame=$frame; timestamp_ms=100*$frame; side=$side
            visibility='visible'; eye_state=$state; occluder='none'; quality='accepted'
            annotator_id=$annotator; notes=''; relative_path="C01/$side/$frame.png"
            candidate_class='synthetic'
        }
    }
    $a = @(Eye 0 left open a; Eye 0 right open a; Eye 300 left closed a; Eye 300 right open a;
        Eye 600 left open a; Eye 600 right open a)
    $b = @(Eye 0 left open b; Eye 0 right open b; Eye 300 left transition b; Eye 300 right open b;
        Eye 600 left open b; Eye 600 right open b)
    $a | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $aDir 'labels-test.csv')
    $b | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $bDir 'labels-test.csv')
    @(
        [pscustomobject]@{frame=0;timestamp_ms=0;temporal_eye_state='open';eye_openness=1;eye_usability='usable';perclos='';perclos_coverage=0},
        [pscustomobject]@{frame=300;timestamp_ms=30000;temporal_eye_state='closed';eye_openness=0;eye_usability='usable';perclos='';perclos_coverage=0.5},
        [pscustomobject]@{frame=600;timestamp_ms=60000;temporal_eye_state='open';eye_openness=1;eye_usability='usable';perclos=0.5;perclos_coverage=1}
    ) | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $traces 'C01.csv')
    @([pscustomobject]@{
        batch_id='test';clip_id='C01';frame=300;timestamp_ms=30000;side='left';relative_path='C01/left/300.png'
        a_visibility='visible';b_visibility='visible';a_eye_state='closed';b_eye_state='transition'
        a_occluder='none';b_occluder='none';a_quality='accepted';b_quality='accepted';a_notes='';b_notes=''
        disagree_visibility=0;disagree_eye_state=1;disagree_occluder=0;disagree_quality=0
        adjudicated_visibility='visible';adjudicated_eye_state='closed';adjudicated_occluder='none'
        adjudicated_quality='accepted';adjudicator_notes='synthetic decision'
    }) | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $root 'adjudication.csv')

    & (Join-Path $PSScriptRoot '..\scripts\score_dense_eye_roi_labels.ps1') `
        -AnnotatorADirectory $aDir -AnnotatorBDirectory $bDir `
        -AdjudicationCsv (Join-Path $root 'adjudication.csv') `
        -TraceDirectory $traces -OutputDirectory $output | Out-Null

    $agreement = Import-Csv -LiteralPath (Join-Path $output 'dense-eye-agreement.csv')
    $summary = Import-Csv -LiteralPath (Join-Path $output 'dense-eye-state-summary.csv') |
        Where-Object batch_id -eq 'ALL'
    $perclos = Import-Csv -LiteralPath (Join-Path $output 'dense-eye-perclos-summary.csv')
    if ($agreement.adjudicated_rows -ne '1' -or $agreement.eye_state_agreement -ne '0.833333') {
        throw 'Unexpected agreement result'
    }
    if ($summary.sampled_pairs -ne '3' -or $summary.truth_evaluable -ne '3' -or
        $summary.conditional_accuracy -ne '1' -or $summary.end_to_end_accuracy -ne '1') {
        throw 'Unexpected dense state score'
    }
    if ($perclos.comparable_windows -ne '1' -or $perclos.mean_absolute_error -ne '0' -or
        $perclos.mean_truth_perclos -ne '0.5' -or $perclos.mean_model_perclos -ne '0.5') {
        throw 'Unexpected fixed-window PERCLOS score'
    }
    Write-Output 'dense eye ROI scoring test PASSED'
} finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
