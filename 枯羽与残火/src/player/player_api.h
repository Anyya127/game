/**
 * @file player_api.h
 * @brief 玩家模块公开接口
 *
 * 职责：玩家创建/销毁、输入驱动的移动/跳跃/射击、状态查询、战斗接口
 * 依赖：types.h
 */

#ifndef PLAYER_API_H
#define PLAYER_API_H

#include "../types/types.h"
#include "raylib.h"

// ==================== 生命周期 ====================
void   Player_Init(void);
void   Player_Shutdown(void);
Entity Player_Create(float x, float y);
void   Player_Destroy(Entity e);

// ==================== 每帧更新 ====================
// 输入驱动移动/跳跃/射击，进行平台碰撞检测，返回最新玩家数据
PlayerData Player_Update(Entity e, PlayerInput input, float dt,
                          const PlatformData* platforms, int platformCount);

// ==================== 查询 ====================
PlayerData Player_GetData(Entity e);
Entity     Player_GetEntity(void);
Rect       Player_GetBounds(Entity e);
bool       Player_IsDead(Entity e);

// ==================== 战斗接口（供 combat 模块调用） ====================
void Player_TakeDamage(Entity e, int damage);
void Player_AddScore(Entity e, int points);
void Player_Reset(Entity e, float x, float y);

#endif
