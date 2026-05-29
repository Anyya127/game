/**
 * @file combat_api.h
 * @brief 战斗模块公开接口
 *
 * 职责：
 * - 子弹管理（创建、移动、销毁）
 * - 碰撞检测（子弹 vs 敌人、子弹 vs 玩家、敌人 vs 玩家接触伤害）
 * - 战斗事件队列（供 render 模块消费驱动表现，供 main.c 消费驱动扣血）
 * - 连击计数
 *
 * 依赖：types.h
 */

#ifndef COMBAT_API_H
#define COMBAT_API_H

#include "../types/types.h"

// ==================== 生命周期 ====================
void Combat_Init(void);
void Combat_Shutdown(void);

// ==================== 子弹管理 ====================
Entity Bullet_Create(float x, float y, float vx, float vy,
                      BulletType type, int damage);
void   Bullet_UpdateAll(float dt);
void   Bullet_Destroy(Entity e);
void   Bullet_ClearAll(void);
int    Bullet_Count(void);
void   Bullet_GetAll(BulletData* out, int maxCount);

// ==================== 每帧判定 ====================
// 传入玩家和敌人/子弹列表，执行全部碰撞检测，结果写入内部事件队列
void Combat_Update(Entity playerEntity,
                   const EnemyData* enemies, int enemyCount,
                   const BulletData* bullets, int bulletCount,
                   float dt);

// ==================== 事件队列 ====================
int  Combat_PollEvents(Event* out, int maxEvents);
void Combat_ClearEvents(void);

// ==================== 连击 ====================
int Combat_ComboCount(void);

#endif
