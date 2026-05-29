# =============================================================
#  Enchanted Warrior - One-Click Setup (Visual Studio Edition)
#
#  How to run (as Administrator in PowerShell):
#    powershell -ExecutionPolicy Bypass -File .\scripts\setup.ps1
#
#  What this does:
#    1. Checks winget / Git (installs if missing)
#    2. git pull to update code
#    3. Detects Visual Studio (2019/2022/2025/2026)
#    4. Initializes MSVC toolchain (vcvars64.bat)
#    5. Runs cmake + msbuild
#    6. Generates .sln for Visual Studio
#    7. Prints all paths, asks whether to launch
# =============================================================

$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $PSScriptRoot

# ---- Check Admin ----
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "[ERROR] Please run PowerShell as Administrator" -ForegroundColor Red
    Write-Host "        Shortcut: Win+X -> Terminal(Admin)" -ForegroundColor Yellow
    return
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Enchanted Warrior - One-Click Setup" -ForegroundColor Cyan
Write-Host "  Project: $ProjectDir" -ForegroundColor DarkGray
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# ---- Helpers ----
function Refresh-Path {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path","User")
}

function Test-Cmd($name) {
    return (Get-Command $name -ErrorAction SilentlyContinue) -ne $null
}

function Install-IfMissing($name, $wingetPkg, $displayName) {
    if (Test-Cmd $name) {
        Write-Host "       $displayName [OK] (already installed)" -ForegroundColor DarkGreen
        return $true
    }
    Write-Host "       Installing $displayName..." -ForegroundColor Yellow
    winget install $wingetPkg --accept-package-agreements --silent 2>&1 | Out-Null

    for ($retry = 0; $retry -lt 5; $retry++) {
        Start-Sleep -Seconds 2
        Refresh-Path
        if (Test-Cmd $name) {
            Write-Host "       $displayName [OK] (installed)" -ForegroundColor DarkGreen
            return $true
        }
    }

    Write-Host "       $displayName installed but PATH not refreshed yet" -ForegroundColor Yellow
    Write-Host "       Close this window, re-open terminal as Admin, and run this script again" -ForegroundColor Yellow
    return $false
}

function Find-VS {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath 2>$null
        if ($vsPath) { return $vsPath }
    }
    $years = @(2026, 2025, 2022, 2019)
    $editions = @("Community", "Professional", "Enterprise", "BuildTools")
    foreach ($year in $years) {
        foreach ($ed in $editions) {
            $p = "${env:ProgramFiles}\Microsoft Visual Studio\$year\$ed"
            if (Test-Path "$p\VC\Auxiliary\Build\vcvars64.bat") { return $p }
            $p = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\$year\$ed"
            if (Test-Path "$p\VC\Auxiliary\Build\vcvars64.bat") { return $p }
        }
    }
    return $null
}

# =============================================================
# Step 1: winget
# =============================================================
Write-Host "[1/5] Checking winget..." -ForegroundColor Green
if (-not (Test-Cmd winget)) {
    Write-Host "       winget not found" -ForegroundColor Red
    Write-Host "       Requires Windows 10 1809+ or Windows 11" -ForegroundColor Yellow
    return
}
Write-Host "       winget [OK]" -ForegroundColor DarkGreen

# =============================================================
# Step 2: Git
# =============================================================
Write-Host "[2/5] Checking Git..." -ForegroundColor Green
if (-not (Install-IfMissing git Git.Git "Git")) {
    Write-Host "       Re-open terminal as Admin and re-run this script" -ForegroundColor Yellow
    return
}

# =============================================================
# Step 3: Pull code
# =============================================================
Write-Host "[3/5] Pulling latest code..." -ForegroundColor Green
Push-Location $ProjectDir
$remote = & git remote get-url origin 2>$null
if ($remote) {
    Write-Host "       Remote: $remote" -ForegroundColor DarkGray
    & git pull --ff-only 2>&1 | Out-Null
    Write-Host "       [OK] Up to date" -ForegroundColor DarkGreen
} else {
    Write-Host "       [WARN] Not a git repo, skipping pull" -ForegroundColor Yellow
}
Write-Host "       Code: $ProjectDir" -ForegroundColor DarkGreen

# =============================================================
# Step 4: Visual Studio
# =============================================================
Write-Host "[4/5] Finding Visual Studio..." -ForegroundColor Green

$vsPath = Find-VS
if (-not $vsPath) {
    Write-Host "       [ERROR] Visual Studio not found" -ForegroundColor Red
    Write-Host ""
    Write-Host "       Install Visual Studio 2022 Community (free):" -ForegroundColor Yellow
    Write-Host "       https://visualstudio.microsoft.com/downloads/" -ForegroundColor Yellow
    Write-Host "       IMPORTANT: Select 'Desktop development with C++' workload" -ForegroundColor Yellow
    Pop-Location
    return
}
Write-Host "       Visual Studio [OK]" -ForegroundColor DarkGreen
Write-Host "       Path: $vsPath" -ForegroundColor DarkGray

# =============================================================
# Step 5: Build
# =============================================================
Write-Host "[5/5] Building..." -ForegroundColor Green

$vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Host "       [ERROR] vcvars64.bat not found. VS C++ workload may be missing." -ForegroundColor Red
    Pop-Location
    return
}

$buildDir = "$ProjectDir\build"
$slnFile  = "$buildDir\EnchantedWarrior.sln"

if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
}

$buildScript = @"
@echo off
call "$vcvars" >nul 2>&1
cd /d "$ProjectDir"
cmake -S . -B "$buildDir" -DBUILD_EXAMPLES=OFF >nul 2>&1
if %errorlevel% neq 0 (
    echo CMAKE_ERROR
    exit /b 1
)
cmake --build "$buildDir" --config Debug -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo BUILD_ERROR
    exit /b 1
)
echo BUILD_OK
"@

$buildBat = "$env:TEMP\ew_build.bat"
Set-Content -Path $buildBat -Value $buildScript -Encoding ASCII

Write-Host "       CMake + MSBuild running..." -ForegroundColor DarkGray
$buildResult = & cmd /c $buildBat 2>&1
Remove-Item $buildBat -Force -ErrorAction SilentlyContinue

if ($buildResult -match "CMAKE_ERROR") {
    Write-Host "       [ERROR] CMake failed" -ForegroundColor Red
    Pop-Location
    return
}
if ($buildResult -match "BUILD_ERROR") {
    Write-Host "       [ERROR] MSBuild failed" -ForegroundColor Red
    Write-Host $buildResult
    Pop-Location
    return
}

Write-Host "       Build [OK]" -ForegroundColor DarkGreen

# =============================================================
# Summary
# =============================================================
$exe = "$buildDir\Debug\EnchantedWarrior.exe"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Setup Complete!" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Code:          $ProjectDir" -ForegroundColor Yellow
Write-Host "  VS Solution:   $slnFile" -ForegroundColor Yellow
Write-Host "       [Double-click to open in Visual Studio]" -ForegroundColor DarkGray
Write-Host "  Game:          $exe" -ForegroundColor Yellow
Write-Host "       [Double-click to play]" -ForegroundColor DarkGray
Write-Host ""

if (Test-Path $slnFile) {
    Write-Host "  Opening solution folder..." -ForegroundColor Green
    & explorer.exe /select,"$slnFile"
} else {
    & explorer.exe $ProjectDir
}

Write-Host ""

$runGame = Read-Host "Launch game now? (y/n)"
if ($runGame -eq "y" -or $runGame -eq "Y") {
    if (Test-Path $exe) {
        Write-Host "       Launching..." -ForegroundColor Green
        Start-Process $exe
        Write-Host "       Game started!" -ForegroundColor Cyan
    } else {
        Write-Host "       [ERROR] Executable not found" -ForegroundColor Red
    }
} else {
    Write-Host "       Skipped" -ForegroundColor DarkGray
}

Pop-Location

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Next time:" -ForegroundColor White
Write-Host "    VS dev:   Double-click $slnFile" -ForegroundColor Yellow
Write-Host "    Play:     $exe" -ForegroundColor Yellow
Write-Host "    Update:   git pull && re-run this script" -ForegroundColor Yellow
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
