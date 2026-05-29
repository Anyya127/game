# =============================================================
#  枯羽与残火 — 一键启动脚本 (Visual Studio 版)
#
#  队友使用方法（以管理员身份打开 PowerShell，粘贴运行）:
#    irm https://gitee.com/Anyya__127/game/raw/main/scripts/一键启动.ps1 | iex
#
#  脚本自动完成:
#    检查winget → 安装Git → 拉代码 → 安装CMake
#    → 检测Visual Studio → MSVC编译 → 启动游戏
#
#  已安装的环境会自动跳过，不会重复安装
#  因为队友全是 VS 开发，所以直接用 VS 自带的 MSVC 编译器，不用 MinGW
# =============================================================

$ErrorActionPreference = "Stop"

$RepoUrl    = "https://gitee.com/Anyya__127/game.git"
$ProjectDir = "$env:USERPROFILE\枯羽与残火"

# ---- 检查管理员权限 ----
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "[错误] 请以管理员身份运行 PowerShell 再粘贴命令" -ForegroundColor Red
    Write-Host "       快捷方式: Win+X -> 终端(管理员)" -ForegroundColor Yellow
    return
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  枯羽与残火 — 一键启动 (VS版)" -ForegroundColor Cyan
Write-Host "  仓库: $RepoUrl" -ForegroundColor DarkGray
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# ---- 辅助函数：刷新 PATH ----
function Refresh-Path {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("Path","User")
}

# ---- 辅助函数：检查命令是否存在 ----
function Test-Cmd($name) {
    return (Get-Command $name -ErrorAction SilentlyContinue) -ne $null
}

# ---- 辅助函数：如果不存在则用 winget 安装，安装后等待PATH生效 ----
function Install-IfMissing($name, $wingetPkg, $displayName) {
    if (Test-Cmd $name) {
        Write-Host "       $displayName ✓ (已存在)" -ForegroundColor DarkGreen
        return $true
    }
    Write-Host "       正在安装 $displayName..." -ForegroundColor Yellow
    winget install $wingetPkg --accept-package-agreements --silent 2>&1 | Out-Null

    # 安装后 PATH 可能不会立即生效，等一下再反复尝试
    for ($retry = 0; $retry -lt 5; $retry++) {
        Start-Sleep -Seconds 2
        Refresh-Path
        if (Test-Cmd $name) {
            Write-Host "       $displayName ✓ (安装完成)" -ForegroundColor DarkGreen
            return $true
        }
    }

    # 5次都没刷出来，提示用户重启终端
    Write-Host "       $displayName 已安装，但 PATH 未生效" -ForegroundColor Yellow
    Write-Host "       请关闭此窗口，重新以管理员打开终端，再粘贴同一行命令" -ForegroundColor Yellow
    Write-Host "       (第二次运行会直接跳过安装，秒进编译)" -ForegroundColor Yellow
    return $false
}

# ---- 辅助函数：检测 Visual Studio (2019/2022/2025/2026/...) ----
function Find-VS {
    # vswhere.exe 是 VS 官方自带的定位工具，能查所有版本
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        # -latest 找最新版，-requires 确保装了 C++ 工作负载
        $vsPath = & $vswhere -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath 2>$null
        if ($vsPath) {
            return $vsPath
        }
    }
    # 回退：vswhere 不存在或没返回，手动搜常见版本
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
# 步骤1: 检查 winget
# =============================================================
Write-Host "[1/5] 检查包管理器 winget..." -ForegroundColor Green
if (-not (Test-Cmd winget)) {
    Write-Host "       winget 未安装" -ForegroundColor Red
    Write-Host "       请确保系统为 Windows 10 1809+ 或 Windows 11" -ForegroundColor Yellow
    return
}
Write-Host "       winget ✓" -ForegroundColor DarkGreen

# =============================================================
# 步骤2: 检查/安装 Git
# =============================================================
Write-Host "[2/5] 检查 Git..." -ForegroundColor Green
if (-not (Install-IfMissing git Git.Git "Git")) {
    Write-Host "       请关闭此窗口，重新以管理员身份打开终端，" -ForegroundColor Yellow
    Write-Host "       再次粘贴同一行命令即可继续" -ForegroundColor Yellow
    return
}

# =============================================================
# 步骤3: 拉取代码
# =============================================================
Write-Host "[3/5] 拉取代码..." -ForegroundColor Green
if (Test-Path $ProjectDir) {
    Write-Host "       目录已存在，检查更新..."
    Push-Location $ProjectDir
    $remote = & git remote get-url origin 2>$null
    if ($remote -ne $RepoUrl) {
        Write-Host "       [注意] 仓库地址不匹配，使用本地已有代码" -ForegroundColor Yellow
    } else {
        & git pull --ff-only 2>&1 | Out-Null
        Write-Host "       已更新到最新版本 ✓"
    }
} else {
    Write-Host "       正在克隆仓库（约 20MB）..."
    & git clone $RepoUrl $ProjectDir 2>&1
    Push-Location $ProjectDir
    Write-Host "       代码已拉取 ✓"
}

Write-Host "       代码目录: $ProjectDir" -ForegroundColor DarkGreen

# =============================================================
# 步骤4: 检测 Visual Studio
# =============================================================
Write-Host "[4/5] 检测 Visual Studio..." -ForegroundColor Green

$vsPath = Find-VS
if (-not $vsPath) {
    Write-Host "       [错误] 未检测到 Visual Studio 2022" -ForegroundColor Red
    Write-Host ""
    Write-Host "       请安装 Visual Studio 2022 社区版（免费）：" -ForegroundColor Yellow
    Write-Host "       https://visualstudio.microsoft.com/zh-cn/downloads/" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "       安装时务必勾选「使用 C++ 的桌面开发」工作负载" -ForegroundColor Yellow
    Write-Host "       安装完成后重新运行本脚本即可" -ForegroundColor Yellow
    Pop-Location
    return
}
Write-Host "       Visual Studio ✓" -ForegroundColor DarkGreen
Write-Host "       路径: $vsPath" -ForegroundColor DarkGray

# =============================================================
# 步骤5: CMake 配置 + MSVC 编译 + 启动
# =============================================================
Write-Host "[5/5] 编译与启动..." -ForegroundColor Green

# 用 VS 自带的 Developer PowerShell 环境变量来确保 MSVC 工具链可用
$vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    Write-Host "       [错误] 找不到 vcvars64.bat，VS 安装可能不完整" -ForegroundColor Red
    Write-Host "       请确保安装了「使用 C++ 的桌面开发」工作负载" -ForegroundColor Yellow
    Pop-Location
    return
}

# 调用 vcvars 设置 MSVC 环境，然后执行 cmake
# 用 cmd /c 串联：先初始化 MSVC 环境变量，再在同一进程中跑 cmake + msbuild
Write-Host "       初始化 MSVC 编译环境..."
Write-Host "       CMake 配置 + MSBuild 编译中..."

$buildDir = "$ProjectDir\build"
$slnFile  = "$buildDir\EnchantedWarrior.sln"

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

$buildResult = & cmd /c $buildBat 2>&1
Remove-Item $buildBat -Force -ErrorAction SilentlyContinue

if ($buildResult -match "CMAKE_ERROR") {
    Write-Host "       [错误] CMake 配置失败" -ForegroundColor Red
    Pop-Location
    return
}
if ($buildResult -match "BUILD_ERROR") {
    Write-Host "       [错误] MSBuild 编译失败" -ForegroundColor Red
    Write-Host $buildResult
    Pop-Location
    return
}

Write-Host "       编译成功 ✓" -ForegroundColor DarkGreen

# =============================================================
# 打印路径汇总
# =============================================================
$exe = "$buildDir\Debug\EnchantedWarrior.exe"

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  全部完成！" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  代码目录:" -ForegroundColor White
Write-Host "    $ProjectDir" -ForegroundColor Yellow
Write-Host ""
Write-Host "  Visual Studio 解决方案:" -ForegroundColor White
Write-Host "    $slnFile" -ForegroundColor Yellow
Write-Host "    --> 双击此文件即可用 Visual Studio 打开整个项目" -ForegroundColor DarkGray
Write-Host ""
Write-Host "  游戏程序:" -ForegroundColor White
Write-Host "    $exe" -ForegroundColor Yellow
Write-Host "    --> 双击启动游戏" -ForegroundColor DarkGray
Write-Host ""

# 打开 .sln 所在文件夹，选中 .sln 文件
if (Test-Path $slnFile) {
    Write-Host "  正在打开解决方案所在文件夹..." -ForegroundColor Green
    & explorer.exe /select,"$slnFile"
} else {
    & explorer.exe $ProjectDir
}

Write-Host ""

# =============================================================
# 询问是否启动游戏
# =============================================================
$runGame = Read-Host "是否现在启动游戏？(y/n)"
if ($runGame -eq "y" -or $runGame -eq "Y") {
    if (Test-Path $exe) {
        Write-Host "       启动游戏..." -ForegroundColor Green
        Start-Process $exe
        Write-Host "       游戏已启动！" -ForegroundColor Cyan
    } else {
        Write-Host "       [错误] 找不到可执行文件: $exe" -ForegroundColor Red
    }
} else {
    Write-Host "       跳过启动" -ForegroundColor DarkGray
}

Pop-Location

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  下次使用：" -ForegroundColor White
Write-Host ""
Write-Host "    Visual Studio 打开项目:" -ForegroundColor DarkGray
Write-Host "      双击 $slnFile" -ForegroundColor Yellow
Write-Host ""
Write-Host "    直接运行游戏:" -ForegroundColor DarkGray
Write-Host "      $exe" -ForegroundColor Yellow
Write-Host ""
Write-Host "    在 VS 中修改代码后重新编译:" -ForegroundColor DarkGray
Write-Host "      Visual Studio 菜单 -> 生成 -> 生成解决方案 (Ctrl+Shift+B)" -ForegroundColor Yellow
Write-Host ""
Write-Host "    拉取最新代码:" -ForegroundColor DarkGray
Write-Host "      cd $ProjectDir" -ForegroundColor Yellow
Write-Host "      git pull" -ForegroundColor Yellow
Write-Host "      然后打开 .sln 重新生成" -ForegroundColor Yellow
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
