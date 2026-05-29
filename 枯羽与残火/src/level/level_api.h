/**
 * @file level_api.h
 * @brief 关卡/流程模块公开接口
 *
 * 职责：
 * - 关卡数据（平台、地面）加载与查询
 * - 摄像机跟随与震屏
 * - 游戏流程状态机（菜单 → 游戏中 → 暂停 → 死亡）
 * - 敌人生成队列
 *
 * 依赖：types.h
 */

#ifndef LEVEL_API_H
#define LEVEL_API_H

#include "../types/types.h"

// ==================== 生命周期 ====================
void Level_Init(void);
void Level_Shutdown(void);
void Level_Load(int levelId);

// ==================== 每帧更新 ====================
// playerX 驱动摄像机跟随，dt 驱动敌人生成计时
void Level_Update(float playerX, float dt);

// ==================== 平台查询 ====================
int   Level_GetPlatforms(PlatformData* out, int maxCount);
float Level_GetGroundHeight(float x);

// ==================== 摄像机 ====================
float Level_GetScrollX(void);                     // 当前滚动偏移（含震屏抖动）
void  Level_Shake(float intensity, float duration); // 触发震屏

// ==================== 流程状态机 ====================
GameState Level_State(void);
GameUI    Level_UI(void);
void      Level_SetState(GameState s);
void      Level_StartGame(void);
void      Level_Pause(void);
void      Level_Resume(void);
void      Level_GameOver(void);
bool      Level_IsPaused(void);

// ==================== 敌人生成队列 ====================
typedef struct {
    float x, y;
    EnemyType type;
} SpawnRequest;

int  Level_PollSpawns(SpawnRequest* out, int maxCount); // 取出本帧待生成的敌人
int  Level_CurrentId(void);
int  Level_TotalKills(void);
void Level_AddKill(void);

#endif
