# Symlinking OpenCV

param(
    #  Standard windows setup has
    # env::USERPROFILE cannot work in admin powershell
    #[string]$BuildDir = "$env:USERPROFILE\opencv_build",
    [string]$OpenCVBuild = "c:\Users\sbashtav\opencv_build",
    [string]$ThirdPartyDir = "third_party\opencv"
)

# Ensure we're running as Administrator (required for symlinks on Windows)
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script requires Administrator privileges to create symbolic links."
    Write-Host "Please run PowerShell as Administrator and try again." -ForegroundColor Yellow
    exit 1
}

Write-Host "Creating OpenCV symbolic links..." -ForegroundColor Green

# Remove entire third party directory if it exists (clean slate)
if (Test-Path $ThirdPartyDir) {
    Write-Host "Removing existing directory: $ThirdPartyDir" -ForegroundColor Yellow
    Remove-Item $ThirdPartyDir -Recurse -Force
}

# Create fresh third party directory
New-Item -ItemType Directory -Force -Path $ThirdPartyDir | Out-Null

# Create symbolic links
Write-Host "Creating symbolic links..." -ForegroundColor Cyan

# Headers (same for both debug and release)
New-Item -ItemType SymbolicLink -Path "$ThirdPartyDir\include" -Target "$OpenCVBuild\install\release\include" -Force
Write-Host "[OK] Linked: include -> $OpenCVBuild\install\release\include"

New-Item -ItemType SymbolicLink -Path "$ThirdPartyDir\include_debug" -Target "$OpenCVBuild\install\debug\include" -Force
Write-Host "[OK] Linked: include_debug -> $OpenCVBuild\install\debug\include"

# Libraries and binaries
New-Item -ItemType SymbolicLink -Path "$ThirdPartyDir\release" -Target "$OpenCVBuild\install\release" -Force
Write-Host "[OK] Linked: release -> $OpenCVBuild\install\release"

New-Item -ItemType SymbolicLink -Path "$ThirdPartyDir\debug" -Target "$OpenCVBuild\install\debug" -Force
Write-Host "[OK] Linked: debug -> $OpenCVBuild\install\debug"

Write-Host ""
Write-Host "Symbolic links created successfully!" -ForegroundColor Green
Write-Host "Directory structure:" -ForegroundColor Cyan
Write-Host "  $ThirdPartyDir\include -> Release headers"
Write-Host "  $ThirdPartyDir\include_debug -> Debug headers"
Write-Host "  $ThirdPartyDir\release -> Release libs/bins"
Write-Host "  $ThirdPartyDir\debug -> Debug libs/bins"

# Verify the links work
Write-Host ""
Write-Host "Verifying links..." -ForegroundColor Yellow
$LinksToVerify = @(
    "$ThirdPartyDir\include"
    "$ThirdPartyDir\include_debug"
    "$ThirdPartyDir\release"
    "$ThirdPartyDir\debug"
)

foreach ($link in $LinksToVerify) {
    if (Test-Path $link) {
        $target = (Get-Item $link).Target
        Write-Host "[OK] $link -> $target" -ForegroundColor Green
    } else {
        Write-Host "[FAILED] $link - FAILED" -ForegroundColor Red
    }
}