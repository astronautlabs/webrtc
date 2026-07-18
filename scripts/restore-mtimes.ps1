[CmdletBinding()]
param()

# Short-circuit if the tool isn't in the PATH
if (-not (Get-Command "git-restore-mtime" -ErrorAction SilentlyContinue)) {
    Write-Warning "git-restore-mtime is not installed. Skipping mtime restoration."
    Write-Warning "Ninja may trigger unnecessary rebuilds."
    Write-Warning "(To fix this, install git-restore-mtime and ensure it is in your PATH)"
    return
}

Write-Host "Restoring mtimes for all WebRTC git repositories..." -ForegroundColor Cyan

# 1. Traverse in memory, explicitly skipping CIPD binary package caches
$gitDirs = Get-ChildItem -Path . -Recurse -Directory -Filter ".git" -ErrorAction SilentlyContinue |
           Where-Object { $_.FullName -notmatch '\.cipd' }

# 2. Iterate and restore
foreach ($dir in $gitDirs) {
    $repoPath = $dir.Parent.FullName
    Write-Host " -> Fixing mtimes in $repoPath"

    Push-Location $repoPath
    try {
        Start-Process -FilePath "git-restore-mtime" -NoNewWindow -Wait
    } catch {
        Write-Warning "Failed to restore mtime in $repoPath"
    } finally {
        Pop-Location
    }
}

Write-Host "Mtime restoration complete." -ForegroundColor Green
