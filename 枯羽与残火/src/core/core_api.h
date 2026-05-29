/**
 * @file core_api.h
 * @brief 核心模块公开接口
 *
 * 职责：
 * - 窗口初始化与关闭
 * - 帧时间获取与控制
 * - 输入采集（键盘 + 鼠标 → PlayerInput）
 * - 数学工具函数
 *
 * 依赖：types.h, raylib
 */

#ifndef CORE_API_H
#define CORE_API_H

#include "../types/types.h"

// ==================== 窗口 ====================
void Core_InitWindow(int w, int h, const char* title);
bool Core_ShouldClose(void);
void Core_CloseWindow(void);

// ==================== 帧控制 ====================
float Core_DeltaTime(void);
void  Core_BeginFrame(void);
void  Core_EndFrame(void);

// ==================== 输入 ====================
PlayerInput Core_PollInput(void);

// ==================== 工具函数 ====================
float Util_Clamp(float v, float min, float max);
float Util_Lerp(float a, float b, float t);
float Util_Distance(float x1, float y1, float x2, float y2);
int   Util_Random(int min, int max);

#endif
