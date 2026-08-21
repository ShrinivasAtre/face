param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('yunet', 'mediapipe')]
    [string]$Backend
)

$ErrorActionPreference = 'Stop'
$DeployDir = $PSScriptRoot
$Executable = Join-Path $DeployDir 'yunet_demo.exe'
if (-not (Test-Path $Executable -PathType Leaf)) {
    throw "Deployed application not found: $Executable"
}

Push-Location $DeployDir
try {
    & $Executable "--backend=$Backend"
    $exitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}
exit $exitCode
