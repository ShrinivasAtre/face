$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Header = Join-Path $RepoRoot 'third_party\mediapipe\mediapipe\framework\api3\calculator_context.h'
$GraphHeader = Join-Path $RepoRoot 'third_party\mediapipe\mediapipe\framework\api3\graph.h'
$ImageToTensorHeader = Join-Path $RepoRoot 'third_party\mediapipe\mediapipe\calculators\tensor\image_to_tensor_calculator.h'

foreach ($required in @($Header, $GraphHeader, $ImageToTensorHeader)) {
    if (-not (Test-Path $required)) {
        throw "MediaPipe API3 header not found at $required. Run scripts\fetch_mediapipe.ps1 first."
    }
}

function Write-Utf8NoBom([string]$Path, [string]$Content) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

# Newer MSVC rejects the unused non-type template pack after a type parameter
# pack in the packet visitor helpers. The pack is not referenced by these
# helpers, so remove it while preserving behavior.
$content = [System.IO.File]::ReadAllText($Header)
$replacements = @(
    @(
        'template <typename T, int&... DoNotSpecify, typename F>',
        'template <typename T, typename F>'
    ),
    @(
        "template <typename T, typename U, typename... Rest, int&... DoNotSpecify,`r`n          typename F>",
        'template <typename T, typename U, typename... Rest, typename F>'
    ),
    @(
        "template <typename T, typename U, typename... Rest, int&... DoNotSpecify,`n          typename F>",
        'template <typename T, typename U, typename... Rest, typename F>'
    )
)

$changed = $false
foreach ($pair in $replacements) {
    if ($content.Contains($pair[0])) {
        $content = $content.Replace($pair[0], $pair[1])
        $changed = $true
    }
}

if ($content.Contains('template <typename T, int&... DoNotSpecify, typename F>') -or
    $content.Contains('typename... Rest, int&... DoNotSpecify')) {
    throw 'MediaPipe calculator_context.h MSVC compatibility patch did not apply cleanly.'
}
Write-Utf8NoBom $Header $content

# graph.h friends api3::SubgraphContext before that template is declared.
# Newer MSVC can instead bind the unqualified name to the legacy
# mediapipe::SubgraphContext (a non-template), producing C3857 and breaking
# Options<T>() lookup. Forward-declare the API3 template in its own namespace
# before GenericGraph is parsed so the friend declaration is unambiguous.
$graphContent = [System.IO.File]::ReadAllText($GraphHeader)
$namespaceMarker = "namespace mediapipe::api3 {"
$forwardDecl = "template <typename NodeT>`r`nclass SubgraphContext;"
$forwardDeclLf = "template <typename NodeT>`nclass SubgraphContext;"
if (-not $graphContent.Contains($forwardDecl) -and -not $graphContent.Contains($forwardDeclLf)) {
    $replacement = $namespaceMarker + "`r`n`r`n" + $forwardDecl
    if (-not $graphContent.Contains($namespaceMarker)) {
        throw 'Could not find mediapipe::api3 namespace marker in graph.h.'
    }
    $graphContent = $graphContent.Replace($namespaceMarker, $replacement)
    $changed = $true
}

if (-not $graphContent.Contains($forwardDecl) -and -not $graphContent.Contains($forwardDeclLf)) {
    throw 'MediaPipe graph.h SubgraphContext forward declaration patch did not apply cleanly.'
}
Write-Utf8NoBom $GraphHeader $graphContent

# ImageToTensorNode feeds an inline constexpr absl::string_view into the
# CompileTimeString non-type template parameter. Newer MSVC rejects the
# string_view implementation's internal pointer as a constant template value
# (C7658). MediaPipe's Node API explicitly supports a string literal directly,
# so use the literal form while preserving the exact registration name.
$imageContent = [System.IO.File]::ReadAllText($ImageToTensorHeader)
$oldImageNode = 'struct ImageToTensorNode : Node<kImageToTensorNodeName> {'
$newImageNode = 'struct ImageToTensorNode : Node<"ImageToTensorCalculator"> {'
if ($imageContent.Contains($oldImageNode)) {
    $imageContent = $imageContent.Replace($oldImageNode, $newImageNode)
    $changed = $true
}
if (-not $imageContent.Contains($newImageNode)) {
    throw 'MediaPipe ImageToTensorNode MSVC compatibility patch did not apply cleanly.'
}
Write-Utf8NoBom $ImageToTensorHeader $imageContent

if ($changed) {
    Write-Host 'Applied MediaPipe API3 compatibility patches for MSVC.'
} else {
    Write-Host 'MediaPipe API3 compatibility patches already applied.'
}
