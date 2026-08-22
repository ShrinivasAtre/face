param(
    [Parameter(Mandatory = $true)]
    [string]$OutputFile
)

$ErrorActionPreference = 'Stop'
$system = Get-CimInstance Win32_ComputerSystem
$processor = Get-CimInstance Win32_Processor
$operatingSystem = Get-CimInstance Win32_OperatingSystem
$metadata = [ordered]@{
    schema_version = 1
    platform = 'windows-x64'
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
    os_caption = $operatingSystem.Caption
    os_version = $operatingSystem.Version
    os_build = $operatingSystem.BuildNumber
    system_model = $system.Model
    logical_cpu_count = [Environment]::ProcessorCount
    processors = @($processor | ForEach-Object {
        [ordered]@{
            name = $_.Name
            max_clock_mhz = $_.MaxClockSpeed
            cores = $_.NumberOfCores
            logical_processors = $_.NumberOfLogicalProcessors
        }
    })
    cmake_version = (& cmake --version | Select-Object -First 1)
    git_version = (& git --version)
}

$parent = Split-Path -Parent ([System.IO.Path]::GetFullPath($OutputFile))
if ($parent) { New-Item -ItemType Directory -Force $parent | Out-Null }
[System.IO.File]::WriteAllText(
    [System.IO.Path]::GetFullPath($OutputFile),
    (($metadata | ConvertTo-Json -Depth 5) + "`n"),
    [System.Text.UTF8Encoding]::new($false))
