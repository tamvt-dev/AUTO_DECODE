# HyperDecode CLI Auto-Installer for Windows
# This script adds the HyperDecode bin directory to the User PATH.

$BinaryDir = Join-Path $PSScriptRoot "cli\bin"

if (-not (Test-Path $BinaryDir)) {
    Write-Host "Error: Cannot find cli\bin directory. Please run this script from the project root." -ForegroundColor Red
    exit
}

Write-Host "Installing HyperDecode CLI..." -ForegroundColor Cyan
Write-Host "Path: $BinaryDir"

$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")

if ($CurrentPath -like "*$BinaryDir*") {
    Write-Host "HyperDecode is already in your PATH." -ForegroundColor Yellow
} else {
    $NewPath = "$CurrentPath;$BinaryDir"
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "User")
    Write-Host "Successfully added to PATH!" -ForegroundColor Green
}

Write-Host "`nInstallation complete! Please RESTART your terminal to start using 'hyperdecode'." -ForegroundColor Cyan
Write-Host "Tip: You can now type 'hyperdecode --help' from anywhere." -ForegroundColor Gray
