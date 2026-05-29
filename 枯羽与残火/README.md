# 枯羽与残火 (Enchanted Warrior)

横版侧视高速动作游戏。

## 一键启动（队友）

### 一键启动（无需提前安装任何工具）

Win+X → 终端(管理员)，粘贴以下命令（一行一行回车）：

```powershell
Invoke-WebRequest -Uri "https://gitee.com/Anyya__127/game/repository/archive/main.zip" -OutFile "$env:TEMP\game.zip"
Expand-Archive "$env:TEMP\game.zip" -DestinationPath "$env:USERPROFILE" -Force
cd $env:USERPROFILE\game-main
powershell -ExecutionPolicy Bypass -File .\scripts\setup.ps1
```

所有命令都是 PowerShell 自带的，不需要提前装 Git。setup.ps1 会自动检测/安装 Git 和 Visual Studio。

### 如果你还没装 Git

管理员身份打开 PowerShell，粘贴：

```powershell
winget install Git.Git --accept-package-agreements
```

装完后关掉窗口，重新打开终端，粘贴上面三行命令。

## 脚本做了什么

`scripts\setup.ps1` 自动完成：
1. 检测 Visual Studio（支持 2019/2022/2025/2026）
2. 调用 VS 的 MSVC 编译器 + CMake 编译项目
3. 生成 `build\EnchantedWarrior.sln`（双击即可用 VS 打开）
4. 询问是否立即启动游戏

## Visual Studio 二次开发

双击 `build\EnchantedWarrior.sln` 打开项目，在 VS 中：
- 编辑代码 → 生成 → 生成解决方案（Ctrl+Shift+B）
- 调试 → 开始调试（F5）

## 更新代码

```powershell
cd $env:USERPROFILE\枯羽与残火
git pull
cmake --build build --config Debug
```
