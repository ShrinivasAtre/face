param(
    [Parameter(Mandatory=$true)] [string] $Benchmark,
    [int] $Camera = 0,
    [Parameter(Mandatory=$true)] [string] $OutputRoot,
    [string] $PresentationConfig = '',
    [int] $Repeats = 3,
    [int] $Warmup = 30,
    [int] $Frames = 900,
    [int] $SampleMilliseconds = 200
)
$ErrorActionPreference='Stop'
if (!(Test-Path -LiteralPath $Benchmark -PathType Leaf)) { throw "Benchmark not found: $Benchmark" }
if ($PresentationConfig -and !(Test-Path -LiteralPath $PresentationConfig -PathType Leaf)) { throw "Configuration not found: $PresentationConfig" }
if ($Camera -lt 0 -or $Repeats -lt 1 -or $Warmup -lt 1 -or $Frames -lt 1 -or $SampleMilliseconds -lt 10) { throw 'Invalid numeric argument.' }
$root=[IO.Path]::GetFullPath($OutputRoot); New-Item -ItemType Directory -Force -Path $root | Out-Null
$modes=@(@{Name='full';Config=''})
if ($PresentationConfig) { $modes += @{Name='configured';Config=[IO.Path]::GetFullPath($PresentationConfig)} }
foreach($mode in $modes){
  foreach($run in 1..$Repeats){
    $stem='{0}-run-{1:d2}' -f $mode.Name,$run
    $arguments=@("--camera=$Camera",'--backend=yunet',"--warmup=$Warmup","--frames=$Frames",'--resource-profile',"--resource-sample-ms=$SampleMilliseconds","--output=$(Join-Path $root ($stem+'.json'))","--trace=$(Join-Path $root ($stem+'-frames.csv'))","--resource-trace=$(Join-Path $root ($stem+'-resources.csv'))")
    if($mode.Config){$arguments += "--presentation-config=$($mode.Config)"}
    & $Benchmark @arguments
    if($LASTEXITCODE -ne 0){throw "$stem failed with exit code $LASTEXITCODE"}
  }
}
Get-FileHash -Algorithm SHA256 (Get-ChildItem -LiteralPath $root -File) | Export-Csv -NoTypeInformation -Path (Join-Path $root 'SHA256SUMS.csv')
Write-Host "Live resource comparison completed: $root"
