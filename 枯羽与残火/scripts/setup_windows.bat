@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================
echo   枯羽与残火 - Windows 一键环境配置
echo ============================================
echo.

:: ============ 检查管理员权限 ============
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 请右键此文件，选择"以管理员身份运行"
    echo.
    pause
    exit /b 1
)

:: ============ 1. 安装 winget（Windows 10 自带，Win11 自带） ============
echo [1/4] 检查包管理器...
where winget >nul 2>&1
if %errorlevel% neq 0 (
    echo [提示] winget 未安装，正在通过 Microsoft Store 安装...
    start ms-windows-store://pdp/?productid=9NBLGGH4NNS1
    echo 请在弹出的应用商店中点击"安装"，完成后重新运行本脚本
    pause
    exit /b 1
)
echo        winget 已就绪

:: ============ 2. 安装 CMake ============
echo.
echo [2/4] 安装 CMake...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo        正在安装 CMake...
    winget install Kitware.CMake --accept-package-agreements --silent
    if %errorlevel% neq 0 (
        echo [错误] CMake 安装失败，请手动下载: https://cmake.org/download/
        pause
        exit /b 1
    )
    echo        CMake 安装完成！请重新打开命令行窗口后重新运行本脚本
    pause
    exit /b 0
)
echo        CMake 已安装

:: ============ 3. 安装 MinGW-w64（GCC 编译器） ============
echo.
echo [3/4] 安装 MinGW-w64 编译工具链...
where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo        正在安装 MinGW-w64...
    winget install "MSYS2.UCRT64" --accept-package-agreements --silent 2>nul
    if %errorlevel% neq 0 (
        winget install "GNU.MinGW-w64" --accept-package-agreements --silent
    )
    if %errorlevel% neq 0 (
        echo [错误] MinGW 安装失败
        echo        手动下载: https://winlibs.com/ (选 UCRT, x86_64, posix)
        echo        解压后把 bin/ 加到系统 PATH 环境变量
        pause
        exit /b 1
    )
    echo        MinGW 安装完成！
    echo        【重要】请重新打开命令行窗口，然后 cd 到本项目目录，运行：
    echo        cmake -S . -B build -DBUILD_EXAMPLES=OFF -G "MinGW Makefiles"
    echo        cmake --build build -j
    pause
    exit /b 0
)
echo        MinGW (gcc) 已安装

:: ============ 4. 编译项目 ============
echo.
echo [4/4] 编译游戏...
echo.

:: 清理旧的 build
if exist "%~dp0..\build" (
    echo        清理旧的构建目录...
    rmdir /s /q "%~dp0..\build"
)

:: 进入项目根目录
cd /d "%~dp0.."

echo        正在配置 CMake...
cmake -S . -B build -DBUILD_EXAMPLES=OFF -G "MinGW Makefiles" 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [提示] MinGW Makefiles 生成失败，尝试默认生成器...
    cmake -S . -B build -DBUILD_EXAMPLES=OFF 2>&1
    if %errorlevel% neq 0 (
        echo [错误] CMake 配置失败，请检查 MinGW 是否正确安装
        pause
        exit /b 1
    )
)

echo.
echo        正在编译...
cmake --build build -j%NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo [错误] 编译失败，请检查上面的错误信息
    pause
    exit /b 1
)

echo.
echo ============================================
echo    配置完成！
echo.
echo    运行: build\EnchantedWarrior.exe
echo ============================================
echo.

:: 询问是否立即运行
set /p run="是否立即运行游戏？(y/n): "
if /i "%run%"=="y" (
    echo.
    echo 启动游戏...
    start build\EnchantedWarrior.exe
)

pause
