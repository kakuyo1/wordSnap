param(
    [string]$RemoteUrl = "https://github.com/kakuyo1/wordSnap.git",
    [string]$Branch = "main",
    [string]$CommitMessage = "chore: initialize repository and project docs"
)

$ErrorActionPreference = "Stop"

function Assert-GitExists {
    if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
        throw "git is not installed or not in PATH."
    }
}

Assert-GitExists

if (-not (Test-Path ".git")) {
    git init
}

$hasOrigin = git remote | Select-String -SimpleMatch "origin"
if (-not $hasOrigin) {
    git remote add origin $RemoteUrl
}

git add .

$status = git status --porcelain
if ($status) {
    git commit -m $CommitMessage
}

git branch -M $Branch
git push -u origin $Branch

Write-Host "Done. Pushed to $RemoteUrl on branch $Branch"
