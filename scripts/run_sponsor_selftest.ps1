$ErrorActionPreference = 'Stop'
$DeployDir = $PSScriptRoot
& (Join-Path $DeployDir 'dms_sponsor_selftest.exe') (Join-Path $DeployDir 'test-data\synthetic_eye_sequence.csv')
exit $LASTEXITCODE
