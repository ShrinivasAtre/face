param(
    [Parameter(Mandatory = $true)]
    [string] $OutputPath
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$safeRepoRoot = $repoRoot -replace '\\', '/'
$versionFile = Join-Path $repoRoot 'mediapipe\MEDIAPIPE_VERSION'
$bazelFile = Join-Path $repoRoot '.bazelversion'
$modelContract = Join-Path $repoRoot 'mediapipe\FaceLandmarkerModel.cmake'

foreach ($required in @($versionFile, $bazelFile, $modelContract)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required repository contract is missing: $required"
    }
}

$mediaPipe = ConvertFrom-StringData (Get-Content -Raw -LiteralPath $versionFile)
$bazelVersion = (Get-Content -Raw -LiteralPath $bazelFile).Trim()
$modelContractText = Get-Content -Raw -LiteralPath $modelContract
$taskHashMatch = [regex]::Match($modelContractText, '"([0-9a-f]{64})"')
if (-not $taskHashMatch.Success) {
    throw 'Could not read the MediaPipe task-model SHA-256 contract.'
}

$models = @(
    @{
        name = 'YuNet face detector model'
        path = 'models\face_detection_yunet_2026may.onnx'
        expected = 'ebafce4e3c118d6554634be5c27ab333b4c047a9a8c3faf1d7cf93101c22f0f0'
        license = 'MIT'
        source = 'https://github.com/opencv/opencv_zoo/tree/47534e27c9851bb1128ccc0102f1145e27f23f98/models/face_detection_yunet'
        provenance = 'exact-open-cv-zoo-lfs-identity'
    },
    @{
        name = 'OpenCV LBF landmark model'
        path = 'models\lbfmodel.yaml'
        expected = '70dd8b1657c42d1595d6bd13d97d932877b3bed54a95d3c4733a0f740d1fd66b'
        license = 'NOASSERTION'
        source = 'https://github.com/kurnianggoro/GSOC2017/blob/master/data/lbfmodel.yaml'
        provenance = 'candidate-source-recorded-rights-unresolved'
    },
    @{
        name = 'MediaPipe Face Landmarker task model'
        path = 'models\mediapipe\face_landmarker.task'
        expected = $taskHashMatch.Groups[1].Value
        license = 'NOASSERTION'
        source = 'https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task'
        provenance = 'published-url-recorded-rights-unresolved'
    }
)

$components = [System.Collections.Generic.List[object]]::new()
foreach ($model in $models) {
    $path = Join-Path $repoRoot $model.path
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required model is missing: $path"
    }
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()
    if ($actual -ne $model.expected) {
        throw "Model checksum mismatch for $($model.name): expected $($model.expected), got $actual"
    }
    $components.Add([ordered]@{
        type = 'machine-learning-model'
        name = $model.name
        version = 'repository-pinned'
        hashes = @(@{ alg = 'SHA-256'; content = $actual })
        licenses = @(@{ license = @{ name = $model.license } })
        externalReferences = @(@{ type = 'distribution'; url = $model.source })
        properties = @(
            @{ name = 'dms-next:repository-path'; value = ($model.path -replace '\\', '/') },
            @{ name = 'dms-next:provenance-status'; value = $model.provenance }
        )
    })
}

$components.Add([ordered]@{
    type = 'library'
    name = 'MediaPipe'
    version = $mediaPipe.MEDIAPIPE_TAG
    'bom-ref' = "pkg:github/google-ai-edge/mediapipe@$($mediaPipe.MEDIAPIPE_COMMIT)"
    licenses = @(@{ license = @{ id = 'Apache-2.0' } })
    externalReferences = @(@{ type = 'vcs'; url = "https://github.com/google-ai-edge/mediapipe/tree/$($mediaPipe.MEDIAPIPE_COMMIT)" })
    properties = @(@{ name = 'dms-next:git-commit'; value = $mediaPipe.MEDIAPIPE_COMMIT })
})
$components.Add([ordered]@{
    type = 'application'
    name = 'Bazel'
    version = $bazelVersion
    licenses = @(@{ license = @{ id = 'Apache-2.0' } })
    externalReferences = @(@{ type = 'website'; url = "https://github.com/bazelbuild/bazel/tree/$bazelVersion" })
})
$components.Add([ordered]@{
    type = 'library'
    name = 'OpenCV and opencv_contrib'
    version = '4.8.0-validated-baseline'
    licenses = @(@{ license = @{ id = 'Apache-2.0' } })
    externalReferences = @(
        @{ type = 'vcs'; url = 'https://github.com/opencv/opencv/tree/4.8.0' },
        @{ type = 'vcs'; url = 'https://github.com/opencv/opencv_contrib/tree/4.8.0' }
    )
    properties = @(@{ name = 'dms-next:pin-status'; value = 'build-resolved-not-enforced-by-CMake' })
})

$revision = (& git -c "safe.directory=$safeRepoRoot" -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or -not $revision) {
    throw 'Could not determine Git revision.'
}
$porcelain = @(& git -c "safe.directory=$safeRepoRoot" -C $repoRoot status --porcelain)
if ($LASTEXITCODE -ne 0) {
    throw 'Could not determine Git working-tree state.'
}
$dirty = $porcelain.Count -gt 0

$serialSeed = "$revision|$($mediaPipe.MEDIAPIPE_COMMIT)|$bazelVersion|" +
    (($models | ForEach-Object { $_.expected }) -join '|')
$sha = [Security.Cryptography.SHA256]::Create()
$serialHash = [BitConverter]::ToString(
    $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($serialSeed))
).Replace('-', '').ToLowerInvariant()
$sha.Dispose()

$bom = [ordered]@{
    bomFormat = 'CycloneDX'
    specVersion = '1.5'
    serialNumber = "urn:uuid:$($serialHash.Substring(0,8))-$($serialHash.Substring(8,4))-$($serialHash.Substring(12,4))-$($serialHash.Substring(16,4))-$($serialHash.Substring(20,12))"
    version = 1
    metadata = [ordered]@{
        component = [ordered]@{
            type = 'application'
            name = 'DMS Next'
            version = $revision
            licenses = @(@{ license = @{ name = 'NOASSERTION' } })
            properties = @(
                @{ name = 'dms-next:git-dirty'; value = $dirty.ToString().ToLowerInvariant() },
                @{ name = 'dms-next:scope'; value = 'source-inventory-not-final-package-sbom' }
            )
        }
    }
    components = $components
}

$absoluteOutput = [IO.Path]::GetFullPath($OutputPath)
$outputDirectory = Split-Path -Parent $absoluteOutput
if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory | Out-Null
}
$bom | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $absoluteOutput -Encoding utf8
Write-Host "Wrote source SBOM: $absoluteOutput"
