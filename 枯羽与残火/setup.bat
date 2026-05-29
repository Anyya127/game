@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 请右键此文件，选择"以管理员身份运行"
    echo       或者：选中此文件，按 Ctrl+Shift+Enter
    pause
    exit /b 1
)

echo ============================================
echo   枯羽与残火 - 一键安装编译启动
echo ============================================
echo.

set "PROJECT_DIR=%~dp0"
set "VS_PATH="

echo [1/3] 查找 Visual Studio...
echo.

:: 用 vswhere 查找 VS
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath 2^>nul') do (
        set "VS_PATH=%%i"
    )
)

:: 回退手动查找
if "%VS_PATH%"=="" (
    for %%y in (2026 2025 2022 2019) do (
        for %%e in (Community Professional Enterprise BuildTools) do (
            if exist "%ProgramFiles%\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvars64.bat" (
                set "VS_PATH=%ProgramFiles%\Microsoft Visual Studio\%%y\%%e"
                goto :vs_found
            )
            if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%%y\%%e\VC\Auxiliary\Build\vcvars64.bat" (
                set "VS_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\%%y\%%e"
                goto :vs_found
            )
        )
    )
)

:vs_found
if "%VS_PATH%"=="" (
    echo [错误] 未找到 Visual Studio
    echo.
    echo 请安装 Visual Studio 2022 社区版（免费）
    echo https://visualstudio.microsoft.com/zh-cn/downloads/
    echo 安装时务必勾选"使用 C++ 的桌面开发"
    echo.
    pause
    exit /b 1
)

echo       Visual Studio 已找到
echo       %VS_PATH%
echo.

:: 初始化 MSVC 环境
echo [2/3] 初始化 MSVC 编译器...
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] MSVC 初始化失败，请确认安装了 C++ 工作负载
    pause
    exit /b 1
)
echo       MSVC 就绪
echo.

:: 检查 cmake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未找到 CMake。请安装 CMake：
    echo       https://cmake.org/download/
    echo       下载 Windows x64 Installer，安装时勾选"Add CMake to system PATH"
    pause
    exit /b 1
)

:: 编译
echo [3/3] 编译项目...
echo.

set "BUILD_DIR=%PROJECT_DIR%build"
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

echo       CMake 配置中...
cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -DBUILD_EXAMPLES=OFF >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

echo       编译中...
cmake --build "%BUILD_DIR%" --config Debug -j %NUMBER_OF_PROCESSORS%
if %errorlevel% neq 0 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo       编译成功！
echo.

:: 打印路径
set "EXE=%BUILD_DIR%\Debug\EnchantedWarrior.exe"
set "SLN=%BUILD_DIR%\EnchantedWarrior.sln"

echo ============================================
echo   全部完成！
echo ============================================
echo.
echo   代码目录:
echo     %PROJECT_DIR%
echo.
echo   Visual Studio 解决方案:
echo     %SLN%
echo     - 双击此文件用 VS 打开项目
echo.
echo   游戏程序:
echo     %EXE%
echo     - 双击启动游戏
echo ============================================
echo.

:: 打开解决方案文件夹
explorer /select,"%SLN%"

:: 询问启动
set /p run="是否现在启动游戏？(y/n): "
if /i "%run%"=="y" (
    echo       启动游戏...
    start "" "%EXE%"
)

echo.
echo 后续使用：
echo   VS 开发：双击 %SLN%
echo   直接玩：双击 %EXE%
echo   更新代码：重新下载解压，覆盖此目录，再运行 setup.bat
echo.

pause
