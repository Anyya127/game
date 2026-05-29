/**
 * core.c — 核心模块实现
 *
 * 张小凡。封装raylib的窗口/输入/数学函数，让其他模块不直接依赖raylib。
 * 将来换引擎的话，只改这个文件就行。
 */

#include "core_api.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>

// ============ 窗口 ============

void Core_InitWindow(int w, int h, const char* title)
{
    InitWindow(w, h, title);       // raylib 窗口初始化
    SetTargetFPS(TARGET_FPS);      // 锁60帧
}

bool Core_ShouldClose(void)
{
    return WindowShouldClose();    // 点了X或者按了关闭
}

void Core_CloseWindow(void)
{
    CloseWindow();
}

// ============ 帧控制 ============

float Core_DeltaTime(void)
{
    // raylib 的 GetFrameTime 返回上一帧耗时（秒）
    // 60fps 时约 0.0167
    return GetFrameTime();
}

void Core_BeginFrame(void)
{
    BeginDrawing();
    ClearBackground(RAYWHITE);     // 白色背景，后续绘制会覆盖
}

void Core_EndFrame(void)
{
    EndDrawing();                  // 把绘制好的画面显示出来
}

// ============ 输入采集 ============
// 把键盘和鼠标按钮映射成 PlayerInput 结构体
// 支持多种按键方案：方向键/WASD/鼠标

PlayerInput Core_PollInput(void)
{
    PlayerInput in = {0};  // 全部初始化为 false

    // 左右移动：方向键 OR WASD
    in.moveLeft  = IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A);
    in.moveRight = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);

    // 跳跃：空格 OR W OR 上方向
    in.jump = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);

    // 射击：Z OR J OR 鼠标左键
    in.shoot = IsKeyDown(KEY_Z) || IsKeyDown(KEY_J)
            || IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    // 弹反：X OR 鼠标右键
    in.parry = IsKeyPressed(KEY_X)
            || IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

    // 瞬移：C键
    in.teleport = IsKeyPressed(KEY_C);

    // 暂停：P OR ESC（用IsKeyPressed防重复触发）
    in.pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);

    // 重启：R（IsKeyDown长按也行）
    in.restart = IsKeyDown(KEY_R);

    return in;
}

// ============ 数学工具箱 ============
// 这些函数组里到处都在用

float Util_Clamp(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

float Util_Lerp(float a, float b, float t)
{
    // 线性插值：a + (b-a)*t
    // t=0返回a，t=1返回b，t=0.5返回中点
    return a + (b - a) * t;
}

float Util_Distance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

int Util_Random(int min, int max)
{
    // 简单的取模随机，游戏里够用了
    return (rand() % (max - min + 1)) + min;
}
