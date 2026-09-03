param(
    [Parameter(Mandatory=$true)] [string] $BatchRoot,
    [Parameter(Mandatory=$true)] [string[]] $SubjectIds
)

$ErrorActionPreference = 'Stop'
if (Test-Path -LiteralPath $BatchRoot) { throw "Refusing to overwrite existing batch root: $BatchRoot" }
if (-not $SubjectIds.Count) { throw 'At least one anonymous subject ID is required' }
if (@($SubjectIds | Sort-Object -Unique).Count -ne $SubjectIds.Count) { throw 'Subject IDs must be unique' }
foreach ($subject in $SubjectIds) {
    if ($subject -notmatch '^S[0-9]{2,}$') { throw "Invalid anonymous subject ID: $subject" }
}

$incoming = New-Item -ItemType Directory -Path (Join-Path $BatchRoot 'incoming')
New-Item -ItemType Directory -Path (Join-Path $BatchRoot 'frozen') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $BatchRoot 'derived') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $BatchRoot 'annotations') | Out-Null

$definitions = @(
    @{suffix='visible-eye-state'; session='visible-01'; slice='visible_eye_state'},
    @{suffix='ir-eye-state'; session='ir-01'; slice='ir_eye_state'},
    @{suffix='clear-glasses'; session='glasses-01'; slice='visible_clear_glasses'},
    @{suffix='eye-occlusion'; session='occlusion-01'; slice='visible_eye_occlusion'}
)
$rows = @(); $clipNumber = 7
foreach ($subject in $SubjectIds) {
    foreach ($definition in $definitions) {
        $filename = "$subject-$($definition.suffix).mp4"
        $rows += [pscustomobject]@{
            clip_id=('C{0:D2}' -f $clipNumber); subject_id=$subject
            session_id=$definition.session; slice=$definition.slice
            expected_filename=$filename; private_source_path=(Join-Path $incoming $filename)
            consent_confirmed=''; recorded=''; sha256=''; bytes=''; notes=''
        }
        $clipNumber++
    }
}
$rows | Export-Csv -NoTypeInformation -Encoding utf8BOM -LiteralPath (Join-Path $BatchRoot 'recording-inventory.csv')
@"
PRIVATE STAGE 20 RECORDING BATCH

1. Put original recordings in the incoming directory using the exact inventory filenames.
2. Set consent_confirmed=yes and recorded=yes only after private confirmation.
3. Do not enter names or identity information in this directory.
4. Leave sha256 and bytes empty; the freeze script fills them.
5. Recordings, crops, annotations, traces, and per-frame results must not be committed.
"@ | Set-Content -Encoding utf8 -LiteralPath (Join-Path $BatchRoot 'PRIVATE-README.txt')
Write-Output "INITIALIZED $($rows.Count) EXPECTED RECORDINGS AT $BatchRoot"
