/**
 * @file enemy_api.h
 * @brief 敌人模块公开接口
 *
 * 职责：敌人创建/销毁、AI 状态机、敌人数组查询
 * 依赖：types.h
 */

#ifndef ENEMY_API_H
#define ENEMY_API_H

#include "../types/types.h"

// ==================== 生命周期 ====================
void   Enemy_Init(void);
void   Enemy_Shutdown(void);
Entity Enemy_Create(float x, float y, EnemyType type);

// ==================== 每帧更新 ====================
// 传入玩家位置用于 AI 决策，传入平台数据用于物理碰撞
void Enemy_UpdateAll(float playerX, float playerY, float dt,
                      const PlatformData* platforms, int platformCount);

// ==================== 战斗接口（供 combat 模块调用） ====================
void Enemy_TakeDamage(Entity e, int damage);
bool Enemy_IsDead(Entity e);
int  Enemy_GetScoreValue(Entity e);

// ==================== 查询 ====================
EnemyData Enemy_GetData(Entity e);
int       Enemy_Count(void);
void      Enemy_GetAll(EnemyData* out, int maxCount);
void      Enemy_ClearAll(void);

// ==================== 生成（供 level 模块调用） ====================
void Enemy_Spawn(float x, float y, EnemyType type);

#endif
