param(
    [Parameter(Mandatory = $true)] [string] $Root
)

$ErrorActionPreference = 'Stop'
$resolvedRepository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$resolvedRoot = [IO.Path]::GetFullPath($Root)
if ($resolvedRoot.StartsWith($resolvedRepository, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Private Stage 21 data root must be outside the Git repository.'
}
if (Test-Path -LiteralPath $resolvedRoot) {
    throw "Refusing to overwrite existing batch root: $resolvedRoot"
}

New-Item -ItemType Directory -Path $resolvedRoot | Out-Null
$conditions = @(
    @{ Kind='enrollment'; Condition='front-normal-01' },
    @{ Kind='enrollment'; Condition='front-normal-02' },
    @{ Kind='enrollment'; Condition='front-normal-03' },
    @{ Kind='enrollment'; Condition='front-dim-01' },
    @{ Kind='enrollment'; Condition='front-bright-01' },
    @{ Kind='enrollment'; Condition='left-normal-01' },
    @{ Kind='enrollment'; Condition='left-dim-01' },
    @{ Kind='enrollment'; Condition='right-normal-01' },
    @{ Kind='enrollment'; Condition='right-dim-01' },
    @{ Kind='enrollment'; Condition='glasses-or-natural-variation-01' },
    @{ Kind='pad-live'; Condition='live-normal-01' },
    @{ Kind='pad-live'; Condition='live-dim-01' },
    @{ Kind='pad-attack'; Condition='print-01' },
    @{ Kind='pad-attack'; Condition='replay-01' }
)

$rows = foreach ($number in 1..5) {
    $participant = 'ID{0:d2}' -f $number
    New-Item -ItemType Directory -Path (Join-Path $resolvedRoot $participant) | Out-Null
    $index = 0
    foreach ($condition in $conditions) {
        $index++
        [pscustomobject]@{
            participant_id = $participant
            capture_id = '{0}-C{1:d2}' -f $participant, $index
            kind = $condition.Kind
            condition = $condition.Condition
            relative_path = ''
            sha256 = ''
            consent_id = ''
        }
    }
}
$rows | Export-Csv -NoTypeInformation -Encoding utf8 -Path (Join-Path $resolvedRoot 'capture_manifest.csv')

$consent = foreach ($number in 1..5) {
    [pscustomobject]@{
        participant_id = 'ID{0:d2}' -f $number
        consent_id = ''
        consent_date = ''
        enrollment_images = 'no'
        retained_images = 'no'
        derived_embeddings = 'no'
        pad_captures = 'no'
        export_import_test = 'no'
        deletion_test = 'no'
        automatic_update = 'no'
    }
}
$consent | Export-Csv -NoTypeInformation -Encoding utf8 -Path (Join-Path $resolvedRoot 'consent_register.csv')
Write-Host "Initialized private Stage 21 batch at $resolvedRoot"
Write-Host 'Complete consent first; then record relative paths and run validate_stage21_identity_batch.py.'
