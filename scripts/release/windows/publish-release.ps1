[CmdletBinding()]
param(
    [string]$Version,

    [string]$Title,

    [string]$Notes,

    [string]$Repo = "kakuyo1/wordSnap",

    [string]$RepoRoot,

    [switch]$DryRun
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

function Invoke-ExternalCapture {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [string]$StepName
    )

    Write-Step $StepName
    $output = & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        Fail "$StepName failed with exit code $LASTEXITCODE"
    }

    return $output
}

function Prompt-OrDefault {
    param(
        [string]$Prompt,
        [string]$DefaultValue
    )

    $raw = Read-Host "$Prompt [$DefaultValue]"
    if ([string]::IsNullOrWhiteSpace($raw)) {
        return $DefaultValue
    }

    return $raw.Trim()
}

$repoRootPath = Resolve-RepoRoot -CustomRepoRoot $RepoRoot
$gh = Resolve-Executable -Candidates @("gh.exe", "gh") -DisplayName "gh"

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Read-Host "Version (SemVer, e.g. 1.2.3)"
}

$Version = $Version.Trim()
if (-not (Test-SemVer -InputVersion $Version)) {
    Fail "Version must follow SemVer, for example: 1.0.0"
}

$defaultTitle = "wordSnap $Version"
if ([string]::IsNullOrWhiteSpace($Title)) {
    $Title = Prompt-OrDefault -Prompt "Release title" -DefaultValue $defaultTitle
} else {
    $Title = $Title.Trim()
}

$defaultHighlights = "Release notes for version $Version"
if ([string]::IsNullOrWhiteSpace($Notes)) {
    $Notes = Prompt-OrDefault -Prompt "Release highlights" -DefaultValue $defaultHighlights
} else {
    $Notes = $Notes.Trim()
}

$distRoot = Join-Path $repoRootPath (Join-Path "dist" "wordSnap-$Version")
$installerName = "wordSnap-$Version-setup.exe"
$shaName = "wordSnap-$Version-setup.sha256.txt"
$installerPath = Join-Path $distRoot $installerName
$shaPath = Join-Path $distRoot $shaName

if (-not (Test-Path -LiteralPath $installerPath)) {
    Fail "Installer not found: $installerPath. Run package-win.ps1 first."
}

if (-not (Test-Path -LiteralPath $shaPath)) {
    Fail "SHA256 file not found: $shaPath. Run package-win.ps1 first."
}

$checksumLine = (Get-Content -LiteralPath $shaPath -TotalCount 1).Trim()
if ([string]::IsNullOrWhiteSpace($checksumLine)) {
    Fail "SHA256 file is empty: $shaPath"
}

$tag = "V$Version"

$notesContent = @"
## Highlights
- $Notes

## Assets
- $installerName
- $shaName

## Checksum (SHA256)
- $checksumLine
"@

$notesFile = Join-Path $distRoot "release-notes.generated.md"
Set-Content -LiteralPath $notesFile -Encoding ascii -Value $notesContent

if ($DryRun) {
    Write-Step "Dry-run mode enabled. Skipping release creation."
    Write-Host "[M4] Repo:    $Repo"
    Write-Host "[M4] Tag:     $tag"
    Write-Host "[M4] Title:   $Title"
    Write-Host "[M4] Notes:   $notesFile"
    Write-Host "[M4] Asset-1: $installerPath"
    Write-Host "[M4] Asset-2: $shaPath"
    exit 0
}

Invoke-External -FilePath $gh -Arguments @("auth", "status") -StepName "Checking gh authentication"

$createArgs = @(
    "release",
    "create",
    $tag,
    $installerPath,
    $shaPath,
    "--repo", $Repo,
    "--title", $Title,
    "--notes-file", $notesFile,
    "--latest"
)

Invoke-External -FilePath $gh -Arguments $createArgs -StepName "Creating GitHub release"

$releaseUrl = Invoke-ExternalCapture -FilePath $gh -Arguments @("release", "view", $tag, "--repo", $Repo, "--json", "url", "-q", ".url") -StepName "Fetching release URL"

Write-Host ""
Write-Host "[M4] Release published." -ForegroundColor Green
Write-Host "[M4] Tag:   $tag"
Write-Host "[M4] URL:   $releaseUrl"
Write-Host "[M4] Notes: $notesFile"
