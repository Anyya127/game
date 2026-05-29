#!/bin/bash
# setup.sh — 安装编译依赖（Ubuntu/Debian）
# 只需要运行一次

set -e

echo "=== 安装系统依赖（raylib 编译所需） ==="

sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libgl1-mesa-dev \
    libasound2-dev \
    libpulse-dev \
    libudev-dev

echo ""
echo "=== 依赖安装完成 ==="
echo "现在可以直接构建："
echo "  cmake -S . -B build"
echo "  cmake --build build -j\$(nproc)"
echo "  ./build/EnchantedWarrior"
