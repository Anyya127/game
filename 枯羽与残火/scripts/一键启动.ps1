# =============================================================
#  枯羽与残火 — Windows 一键拉取/安装/编译/启动
#  使用方法：右键 → 使用 PowerShell 运行  (需要管理员权限)
#  或者：以管理员身份打开 PowerShell，cd 到项目根目录，运行 ./scripts/一键启动.ps1
# =============================================================
param(
    [string]$RepoUrl = "",    # 如果要自动拉代码，填 Git 仓库地址
    [switch]$SkipClone = $false
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot

# ---- 检查管理员权限 ----
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "[错误] 请以管理员身份运行此脚本" -ForegroundColor Red
    Write-Host "       方法1: 右键此文件 → 使用 PowerShell 运行" -ForegroundColor Yellow
    Write-Host "       方法2: Win+X → 终端(管理员) → cd 到项目目录 → .\scripts\一键启动.ps1" -ForegroundColor Yellow
    pause
    exit 1
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  枯羽与残火 — 一键启动脚本" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# =============================================================
# 步骤0: 克隆仓库（如果提供了 RepoUrl）
# =============================================================
if ($RepoUrl -and -not $SkipClone) {
    Write-Host "[0/5] 拉取代码..." -ForegroundColor Green

    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) {
        Write-Host "       Git 未安装，正在安装..." -ForegroundColor Yellow
        winget install Git.Git --accept-package-agreements --silent
        Write-Host "       Git 安装完成，请重新打开终端后重试" -ForegroundColor Yellow
        pause
        exit 0
    }

    $cloneDir = Join-Path $env:USERPROFILE "枯羽与残火"
    if (Test-Path $cloneDir) {
        Write-Host "       目录已存在，跳过克隆: $cloneDir"
        $ProjectRoot = $cloneDir
    } else {
        git clone $RepoUrl $cloneDir
        $ProjectRoot = $cloneDir
        Write-Host "       代码已拉取到: $cloneDir"
    }
}

Set-Location $ProjectRoot
Write-Host "       项目根目录: $ProjectRoot"

# =============================================================
# 步骤1: 检查/安装 winget
# =============================================================
Write-Host "[1/5] 检查包管理器 winget..." -ForegroundColor Green
$winget = Get-Command winget -ErrorAction SilentlyContinue
if (-not $winget) {
    Write-Host "       winget 未安装，尝试通过 Microsoft Store 安装..." -ForegroundColor Yellow
    Start-Process "ms-windows-store://pdp/?productid=9NBLGGH4NNS1"
    Write-Host "       请在应用商店点击'安装'，完成后重新运行此脚本" -ForegroundColor Yellow
    pause
    exit 0
}
Write-Host "       winget ✓" -ForegroundColor DarkGreen

# =============================================================
# 步骤2: 检查/安装 CMake
# =============================================================
Write-Host "[2/5] 检查 CMake..." -ForegroundColor Green
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "       正在安装 CMake..." -ForegroundColor Yellow
    winget install Kitware.CMake --accept-package-agreements --silent 2>&1 | Out-Null

    # winget 安装后需要刷新 PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if (-not $cmake) {
        Write-Host "       [错误] CMake 安装失败" -ForegroundColor Red
        Write-Host "       请手动下载: https://cmake.org/download/" -ForegroundColor Yellow
        pause
        exit 1
    }
    Write-Host "       CMake 安装完成 ✓" -ForegroundColor DarkGreen
} else {
    Write-Host "       CMake ✓ ($($cmake.Source))" -ForegroundColor DarkGreen
}

# =============================================================
# 步骤3: 检查/安装 MinGW-w64 (GCC)
# =============================================================
Write-Host "[3/5] 检查 GCC (MinGW-w64)..." -ForegroundColor Green

# 刷新 PATH，确保刚装的工具能找到
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gcc) {
    Write-Host "       正在安装 MinGW-w64..." -ForegroundColor Yellow

    # 尝试安装
    $result = winget install "MSYS2.UCRT64" --accept-package-agreements 2>&1
    Write-Host "       $result"

    # 刷新 PATH
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

    # 检查 MSYS2 安装路径
    $mingwPaths = @(
        "C:\msys64\ucrt64\bin",
        "$env:ProgramFiles\msys64\ucrt64\bin",
        "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\*MSYS2*\ucrt64\bin"
    )

    $found = $false
    foreach ($p in $mingwPaths) {
        $gccPath = Join-Path $p "gcc.exe"
        if (Test-Path $gccPath) {
            Write-Host "       找到 MinGW: $p" -ForegroundColor DarkGreen
            $env:Path = "$p;$env:Path"
            $found = $true
            break
        }
    }

    if (-not $found) {
        Write-Host "       [备选] 通过 winget 安装独立 MinGW..." -ForegroundColor Yellow
        winget install "GNU.MinGW-w64" --accept-package-agreements 2>&1 | Out-Null
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    }

    $gcc = Get-Command gcc -ErrorAction SilentlyContinue
    if (-not $gcc) {
        Write-Host "       [错误] MinGW 安装失败" -ForegroundColor Red
        Write-Host "       手动安装: https://winlibs.com/ (选 UCRT, x86_64, posix)" -ForegroundColor Yellow
        Write-Host "       下载后解压，把 bin/ 目录加入系统 PATH，然后重新运行" -ForegroundColor Yellow
        pause
        exit 1
    }

    Write-Host "       MinGW 安装完成 ✓" -ForegroundColor DarkGreen
} else {
    Write-Host "       GCC ✓ ($($gcc.Source))" -ForegroundColor DarkGreen
}

# 验证 GCC 版本
$gccVer = & gcc --version | Select-Object -First 1
Write-Host "       $gccVer"

# =============================================================
# 步骤4: 编译
# =============================================================
Write-Host "[4/5] 编译项目..." -ForegroundColor Green

# 清理旧的 build
if (Test-Path "$ProjectRoot\build") {
    Remove-Item -Recurse -Force "$ProjectRoot\build"
    Write-Host "       已清理旧的构建目录"
}

# CMake 配置
Write-Host "       CMake 配置中..."
$cmakeOutput = & cmake -S $ProjectRoot -B "$ProjectRoot\build" -DBUILD_EXAMPLES=OFF -G "MinGW Makefiles" 2>&1
if ($LASTEXITCODE -ne 0) {
    # 回退到默认生成器
    Write-Host "       MinGW Makefiles 失败，尝试默认生成器..."
    $cmakeOutput = & cmake -S $ProjectRoot -B "$ProjectRoot\build" -DBUILD_EXAMPLES=OFF 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[错误] CMake 配置失败:" -ForegroundColor Red
        Write-Host $cmakeOutput
        pause
        exit 1
    }
}

# 编译
Write-Host "       编译中..."
$procCount = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
& cmake --build "$ProjectRoot\build" -j $procCount 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "[错误] 编译失败" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "       编译成功 ✓" -ForegroundColor DarkGreen

# =============================================================
# 步骤5: 启动
# =============================================================
Write-Host "[5/5] 启动游戏..." -ForegroundColor Green

$exePath = "$ProjectRoot\build\EnchantedWarrior.exe"
if (Test-Path $exePath) {
    Write-Host "       运行: $exePath" -ForegroundColor DarkGreen
    Start-Process $exePath
} else {
    Write-Host "[错误] 找不到可执行文件: $exePath" -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "游戏已启动！" -ForegroundColor Cyan
Write-Host ""
pause
