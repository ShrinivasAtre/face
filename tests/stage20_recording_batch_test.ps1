$ErrorActionPreference='Stop'
$root=Join-Path ([IO.Path]::GetTempPath()) ('stage20-recordings-'+[guid]::NewGuid())
try {
    & (Join-Path $PSScriptRoot '..\scripts\initialize_stage20_recording_batch.ps1') -BatchRoot $root -SubjectIds S03,S04,S05 | Out-Null
    $rows=@(Import-Csv -LiteralPath (Join-Path $root 'recording-inventory.csv'))
    if($rows.Count-ne12-or@($rows.clip_id|Sort-Object -Unique).Count-ne12){throw 'Unexpected inventory'}
    foreach($name in 'incoming','frozen','derived','annotations'){if(-not(Test-Path -LiteralPath (Join-Path $root $name))){throw "Missing $name"}}
    $failed=$false
    try { & (Join-Path $PSScriptRoot '..\scripts\freeze_stage20_recording_batch.ps1') -BatchRoot $root | Out-Null } catch { $failed=$true }
    if(-not$failed){throw 'Freeze accepted incomplete/unconsented recordings'}
    Write-Output 'stage20 recording batch test PASSED'
} finally {if(Test-Path -LiteralPath $root){Remove-Item -LiteralPath $root -Recurse -Force}}
