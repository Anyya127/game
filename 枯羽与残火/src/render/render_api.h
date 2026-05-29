/**
 * @file render_api.h
 * @brief 渲染/表现模块公开接口
 *
 * 职责：
 * - 纹理资源加载/卸载/绘制
 * - 音频资源加载/播放/控制
 * - UI 绘制（面板、菜单、暂停、游戏结束）
 * - 背景与视差滚动
 * - 震屏、hitstop 等表现效果
 * - 消费战斗事件队列驱动表现
 *
 * 依赖：types.h, raylib
 */

#ifndef RENDER_API_H
#define RENDER_API_H

#include "../types/types.h"
#include "raylib.h"

// ==================== 资源初始化 ====================
void Render_Init(const char* resourceDir);  // NULL 表示自动搜索路径
void Render_Shutdown(void);
void Render_UpdateAudio(void);             // 每帧调用，驱动流式音乐

// ==================== 精灵绘制 ====================
void Render_Player(PlayerData* data, float scrollX);
void Render_Enemy(const EnemyData* data, float scrollX);
void Render_Bullet(const BulletData* data, float scrollX);
void Render_Platform(const PlatformData* data, float scrollX);

// ==================== 背景 ====================
void Render_GameBackground(float cameraX);
void Render_MenuBackground(void);

// ==================== UI ====================
void Render_UI(int hp, int maxHp, int lives, int score, int level, int kills);
void Render_Menu(void);
void Render_Pause(void);
void Render_GameOver(int score, int kills);

// ==================== 表现效果 ====================
void VFX_ProcessEvents(const Event* events, int count, float dt);
void VFX_Screenshake(float intensity, float duration);
void VFX_Hitstop(int frames);
void VFX_CameraFollow(float targetX);

// ==================== 音频快捷接口 ====================
void Audio_PlayBGM(int bgmType, float volume);
void Audio_PauseBGM(void);
void Audio_ResumeBGM(void);

#endif
