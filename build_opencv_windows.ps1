# OpenCV Windows Build Script
# Builds both Release and Debug versions
# With clang-cl (
# .\build_opencv_windows.ps1 -UseClang
# With MSVC
# .\build_opencv_windows.ps1
# Custom version/paths
# .\build_opencv_windows.ps1 -OpenCVVersion "4.11.0" -BuildDir "C:\opencv_build"
# Need
# choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System


param(
    [string]$OpenCVVersion = "4.11.0",
    [string]$BuildDir = "$env:USERPROFILE\opencv_build",
    [string]$InstallDir = "$env:USERPROFILE\opencv_build\install",
    [switch]$UseClang = $false  # Use MSVC by default
)

# Ensure we stop on errors
$ErrorActionPreference = "Stop"

Write-Host "Building OpenCV $OpenCVVersion for Windows..." -ForegroundColor Green

# Create and enter build directory
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Set-Location $BuildDir

# Download sources if they don't exist
$OpenCVDir = "opencv-$OpenCVVersion"
$ContribDir = "opencv_contrib-$OpenCVVersion"

if (-not (Test-Path $OpenCVDir)) {
    Write-Host "Downloading OpenCV $OpenCVVersion..." -ForegroundColor Yellow
    $OpenCVUrl = "https://github.com/opencv/opencv/archive/refs/tags/$OpenCVVersion.tar.gz"
    Invoke-WebRequest -Uri $OpenCVUrl -OutFile "opencv-$OpenCVVersion.tar.gz"
    tar -xzf "opencv-$OpenCVVersion.tar.gz"
}

if (-not (Test-Path $ContribDir)) {
    Write-Host "Downloading OpenCV Contrib $OpenCVVersion..." -ForegroundColor Yellow
    $ContribUrl = "https://github.com/opencv/opencv_contrib/archive/refs/tags/$OpenCVVersion.tar.gz"
    Invoke-WebRequest -Uri $ContribUrl -OutFile "opencv_contrib-$OpenCVVersion.tar.gz"  
    tar -xzf "opencv_contrib-$OpenCVVersion.tar.gz"
}

# Function to build for specific configuration
function Build-OpenCV {
    param([string]$BuildType, [string]$BuildSuffix)
    
    $BuildPath = "build_$BuildSuffix"
    $InstallPath = "$InstallDir\$BuildSuffix"
    
    Write-Host "Configuring CMake for $BuildType build..." -ForegroundColor Green
    New-Item -ItemType Directory -Force -Path $BuildPath | Out-Null
    Set-Location $BuildPath

    # Base CMake arguments
    $CMakeArgs = @(
        "-DCMAKE_BUILD_TYPE=$BuildType"
        "-DBUILD_LIST=calib3d,core,features2d,highgui,imgcodecs,imgproc,video,videoio,aruco,ml,stitching"
        "-DOPENCV_EXTRA_MODULES_PATH=`"$BuildDir\$ContribDir\modules`""
        "-DWITH_FFMPEG=ON"
        "-DWITH_QT=ON"
        "-DWITH_GTK=OFF" 
        "-DBUILD_SHARED_LIBS=ON"
        "-DCMAKE_INSTALL_PREFIX=`"$InstallPath`""
        "-DOPENCV_GENERATE_PKGCONFIG=OFF"  # Disable pkg-config on Windows
        "-DBUILD_TESTS=OFF"
        "-DBUILD_PERF_TESTS=OFF"
        "-DBUILD_EXAMPLES=OFF"
        "-DWITH_OPENMP=ON"
        "-DWITH_TBB=ON"
        # Windows-specific optimizations
        "-DCMAKE_CXX_FLAGS_RELEASE=`"/O2 /DNDEBUG`""
        "-DCMAKE_C_FLAGS_RELEASE=`"/O2 /DNDEBUG`""
    )

    # Add compiler-specific flags
    if ($UseClang) {
        Write-Host "Using clang-cl compiler..." -ForegroundColor Cyan
        $CMakeArgs += @(
            "-DCMAKE_C_COMPILER=clang-cl"
            "-DCMAKE_CXX_COMPILER=clang-cl"
            "-DCMAKE_C_FLAGS=`"/std:c17`""
            "-DCMAKE_CXX_FLAGS=`"/std:c++20`""
        )
    } else {
        Write-Host "Using MSVC compiler..." -ForegroundColor Cyan
        $CMakeArgs += @(
            "-DCMAKE_C_FLAGS=`"/std:c17`""
            "-DCMAKE_CXX_FLAGS=`"/std:c++20`""
        )
    }

    # Add source directory
    $CMakeArgs += "`"$BuildDir\$OpenCVDir`""

    # Run CMake
    & cmake @CMakeArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }

    Write-Host "Building OpenCV $BuildType..." -ForegroundColor Green
    & cmake --build . --config $BuildType --parallel
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    Write-Host "Installing OpenCV $BuildType..." -ForegroundColor Green
    & cmake --install . --config $BuildType
    if ($LASTEXITCODE -ne 0) { throw "Install failed" }

    Set-Location $BuildDir
}

try {
    # Build Release version
    Build-OpenCV -BuildType "Release" -BuildSuffix "release"
    
    # Build Debug version  
    Build-OpenCV -BuildType "Debug" -BuildSuffix "debug"

    Write-Host ""
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "Release libraries: $InstallDir\release\lib" -ForegroundColor Cyan
    Write-Host "Debug libraries: $InstallDir\debug\lib" -ForegroundColor Cyan
    Write-Host "Headers: $InstallDir\release\include (same for both)" -ForegroundColor Cyan

    # List the built libraries (check if directories exist first)
    Write-Host ""
    if (Test-Path "$InstallDir\release\lib") {
        Write-Host "Release libraries:" -ForegroundColor Yellow
        $releaseLibs = Get-ChildItem "$InstallDir\release\lib\opencv_*.lib" -ErrorAction SilentlyContinue
        if ($releaseLibs) {
            $releaseLibs | ForEach-Object { Write-Host "  $($_.Name)" }
        } else {
            Write-Host "  No .lib files found, checking for DLLs..." -ForegroundColor Gray
            Get-ChildItem "$InstallDir\release\lib\*.dll" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "  $($_.Name)" }
        }
    } else {
        Write-Host "Release lib directory not found. Checking alternate locations..." -ForegroundColor Yellow
        # Check x64 subfolder (common on Windows)
        if (Test-Path "$InstallDir\release\x64") {
            Write-Host "Found x64 directory structure" -ForegroundColor Gray
            Get-ChildItem "$InstallDir\release\x64" -Recurse -Include "*.lib","*.dll" | ForEach-Object { Write-Host "  $($_.FullName)" }
        }
    }

    Write-Host ""
    if (Test-Path "$InstallDir\debug\lib") {
        Write-Host "Debug libraries:" -ForegroundColor Yellow
        $debugLibs = Get-ChildItem "$InstallDir\debug\lib\opencv_*d.lib" -ErrorAction SilentlyContinue
        if ($debugLibs) {
            $debugLibs | ForEach-Object { Write-Host "  $($_.Name)" }
        } else {
            Write-Host "  No debug .lib files found, checking for DLLs..." -ForegroundColor Gray
            Get-ChildItem "$InstallDir\debug\lib\*d.dll" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "  $($_.Name)" }
        }
    } else {
        Write-Host "Debug lib directory not found. Checking alternate locations..." -ForegroundColor Yellow
        if (Test-Path "$InstallDir\debug\x64") {
            Write-Host "Found debug x64 directory structure" -ForegroundColor Gray
            Get-ChildItem "$InstallDir\debug\x64" -Recurse -Include "*d.lib","*d.dll" | ForEach-Object { Write-Host "  $($_.FullName)" }
        }
    }

    # Show what was actually created
    Write-Host ""
    Write-Host "Actual directory structure:" -ForegroundColor Cyan
    if (Test-Path "$InstallDir\release") {
        Write-Host "Release install contents:" -ForegroundColor Gray
        Get-ChildItem "$InstallDir\release" | ForEach-Object { Write-Host "  $($_.Name)" }
    }

} catch {
    Write-Error "Build failed: $_"
    exit 1
}
