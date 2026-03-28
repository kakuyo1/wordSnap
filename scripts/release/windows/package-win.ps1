[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",

    [switch]$RunTests,

    [switch]$IncludeOptionalAssets,

    [switch]$Clean,

    [switch]$KeepStaging,

    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[M4] $Message" -ForegroundColor Cyan
}

function Fail {
    param([string]$Message)
    throw "[M4] $Message"
}

function Test-SemVer {
    param([string]$InputVersion)
    return $InputVersion -match '^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$'
}

function Resolve-RepoRoot {
    param([string]$CustomRepoRoot)

    if (-not [string]::IsNullOrWhiteSpace($CustomRepoRoot)) {
        if (-not (Test-Path -LiteralPath $CustomRepoRoot)) {
            Fail "RepoRoot does not exist: $CustomRepoRoot"
        }
        return (Resolve-Path -LiteralPath $CustomRepoRoot).Path
    }

    return (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..")).Path
}

function Resolve-Executable {
    param(
        [string[]]$Candidates,
        [string]$DisplayName
    )

    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }

        $cmd = Get-Command -Name $candidate -ErrorAction SilentlyContinue
        if ($null -ne $cmd) {
            return $cmd.Source
        }
    }

    Fail "$DisplayName not found. Checked: $($Candidates -join ', ')"
}

function Invoke-External {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$StepName
    )

    Write-Step $StepName
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        Fail "$StepName failed with exit code $LASTEXITCODE"
    }
}

function Resolve-BuiltExecutablePath {
    param(
        [string]$BuildDir,
        [string]$BuildConfig
    )

    $candidates = @(
        (Join-Path $BuildDir "$BuildConfig\wordSnapV1.exe"),
        (Join-Path $BuildDir "wordSnapV1.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    Fail "Cannot find built executable. Checked: $($candidates -join ', ')"
}

function Copy-OptionalDirectory {
    param(
        [string]$SourceRoot,
        [string]$Name,
        [string]$DestinationRoot
    )

    $source = Join-Path $SourceRoot $Name
    if (Test-Path -LiteralPath $source) {
        Write-Step "Copying optional directory: $Name"
        Copy-Item -LiteralPath $source -Destination (Join-Path $DestinationRoot $Name) -Recurse -Force
    }
}

function Get-PngDimensions {
    param([byte[]]$PngBytes)

    if ($PngBytes.Length -lt 24) {
        Fail "PNG file is too short to read dimensions"
    }

    $signature = [byte[]](137, 80, 78, 71, 13, 10, 26, 10)
    for ($i = 0; $i -lt $signature.Length; $i++) {
        if ($PngBytes[$i] -ne $signature[$i]) {
            Fail "Icon source is not a valid PNG file"
        }
    }

    $width = (($PngBytes[16] -shl 24) -bor ($PngBytes[17] -shl 16) -bor ($PngBytes[18] -shl 8) -bor $PngBytes[19])
    $height = (($PngBytes[20] -shl 24) -bor ($PngBytes[21] -shl 16) -bor ($PngBytes[22] -shl 8) -bor $PngBytes[23])

    if ($width -le 0 -or $height -le 0) {
        Fail "PNG dimensions are invalid"
    }

    return @{ Width = [int]$width; Height = [int]$height }
}

function Convert-PngToIco {
    param(
        [string]$PngPath,
        [string]$IcoPath
    )

    if (-not (Test-Path -LiteralPath $PngPath)) {
        Fail "Icon source PNG not found: $PngPath"
    }

    $pngBytes = [System.IO.File]::ReadAllBytes($PngPath)
    $dimensions = Get-PngDimensions -PngBytes $pngBytes
    $widthByte = if ($dimensions.Width -ge 256) { [byte]0 } else { [byte]$dimensions.Width }
    $heightByte = if ($dimensions.Height -ge 256) { [byte]0 } else { [byte]$dimensions.Height }

    $iconDirectory = Split-Path -Parent $IcoPath
    if (-not [string]::IsNullOrWhiteSpace($iconDirectory)) {
        New-Item -ItemType Directory -Path $iconDirectory -Force | Out-Null
    }

    $stream = [System.IO.File]::Open($IcoPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    $writer = New-Object System.IO.BinaryWriter($stream)
    try {
        $writer.Write([UInt16]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]1)
        $writer.Write([byte]$widthByte)
        $writer.Write([byte]$heightByte)
        $writer.Write([byte]0)
        $writer.Write([byte]0)
        $writer.Write([UInt16]1)
        $writer.Write([UInt16]32)
        $writer.Write([UInt32]$pngBytes.Length)
        $writer.Write([UInt32]22)
        $writer.Write($pngBytes)
    } finally {
        $writer.Dispose()
        $stream.Dispose()
    }
}

if (-not (Test-SemVer -InputVersion $Version)) {
    Fail "Version must follow SemVer, for example: 1.0.0"
}

$repoRootPath = Resolve-RepoRoot -CustomRepoRoot $RepoRoot
$buildDir = Join-Path $repoRootPath "build"
$distRoot = Join-Path $repoRootPath (Join-Path "dist" "wordSnap-$Version")
$stagingDir = Join-Path $distRoot "staging"
$installerScript = Join-Path $repoRootPath "installer\windows\wordSnapV1.iss"
$outputBaseName = "wordSnap-$Version-setup"
$installerOutputPath = Join-Path $distRoot "$outputBaseName.exe"
$shaPath = Join-Path $distRoot "wordSnap-$Version-setup.sha256.txt"
$stagingExe = Join-Path $stagingDir "wordSnapV1.exe"
$logoPngPath = Join-Path $repoRootPath "static\icons\wordSnapLogo.png"
$stagingIcoPath = Join-Path $stagingDir "wordSnapLogo.ico"

if ($Clean -and (Test-Path -LiteralPath $distRoot)) {
    Write-Step "Cleaning old dist directory"
    Remove-Item -LiteralPath $distRoot -Recurse -Force
}

if (Test-Path -LiteralPath $stagingDir) {
    Write-Step "Resetting staging directory"
    Remove-Item -LiteralPath $stagingDir -Recurse -Force
}

New-Item -ItemType Directory -Path $distRoot -Force | Out-Null
New-Item -ItemType Directory -Path $stagingDir -Force | Out-Null

$cmake = Resolve-Executable -Candidates @("cmake.exe", "cmake") -DisplayName "cmake"
$windeployqt = Resolve-Executable -Candidates @("windeployqt.exe", "windeployqt6.exe", "windeployqt", "windeployqt6") -DisplayName "windeployqt"
$iscc = Resolve-Executable -Candidates @(
    "ISCC.exe",
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
) -DisplayName "ISCC.exe"

$ctest = $null
if ($RunTests) {
    $ctest = Resolve-Executable -Candidates @("ctest.exe", "ctest") -DisplayName "ctest"
}

if (-not (Test-Path -LiteralPath $installerScript)) {
    Fail "Installer script not found: $installerScript. Complete M4-C first."
}

Invoke-External -FilePath $cmake -Arguments @("-S", $repoRootPath, "-B", $buildDir) -StepName "Configuring CMake"
Invoke-External -FilePath $cmake -Arguments @("--build", $buildDir, "--config", $Config) -StepName "Building project"

if ($RunTests) {
    Invoke-External -FilePath $ctest -Arguments @("--test-dir", $buildDir, "-C", $Config, "--output-on-failure") -StepName "Running tests"
}

$builtExePath = Resolve-BuiltExecutablePath -BuildDir $buildDir -BuildConfig $Config
Write-Step "Copying executable to staging"
Copy-Item -LiteralPath $builtExePath -Destination $stagingExe -Force

Write-Step "Generating installer icon from wordSnapLogo.png"
Convert-PngToIco -PngPath $logoPngPath -IcoPath $stagingIcoPath

if ($IncludeOptionalAssets) {
    Copy-OptionalDirectory -SourceRoot $repoRootPath -Name "dict" -DestinationRoot $stagingDir
    Copy-OptionalDirectory -SourceRoot $repoRootPath -Name "tessdata" -DestinationRoot $stagingDir
    Copy-OptionalDirectory -SourceRoot $repoRootPath -Name "static" -DestinationRoot $stagingDir
} else {
    Write-Step "Skipping optional assets (dict/tessdata/static)"
}

$deployArgs = @("--dir", $stagingDir)
if ($Config -eq "Debug") {
    $deployArgs += "--debug"
} else {
    $deployArgs += "--release"
}
$deployArgs += $stagingExe

Invoke-External -FilePath $windeployqt -Arguments $deployArgs -StepName "Deploying Qt runtime"

$isccArgs = @(
    $installerScript,
    "/DAppVersion=$Version",
    "/DSourceDir=$stagingDir",
    "/DOutputDir=$distRoot",
    "/DOutputBaseFilename=$outputBaseName",
    "/DAppIconIco=$stagingIcoPath"
)

Invoke-External -FilePath $iscc -Arguments $isccArgs -StepName "Compiling installer"

if (-not (Test-Path -LiteralPath $installerOutputPath)) {
    Fail "Installer output not found: $installerOutputPath"
}

$hash = (Get-FileHash -LiteralPath $installerOutputPath -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath $shaPath -Encoding ascii -Value "$hash *$outputBaseName.exe"

if (-not $KeepStaging -and (Test-Path -LiteralPath $stagingDir)) {
    Write-Step "Cleaning staging directory"
    Remove-Item -LiteralPath $stagingDir -Recurse -Force
}

Write-Host ""
Write-Host "[M4] Packaging completed." -ForegroundColor Green
Write-Host "[M4] Installer: $installerOutputPath"
Write-Host "[M4] SHA256:    $shaPath"
