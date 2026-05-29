/**
 * level.c — 关卡/流程模块实现
 *
 * 赵文博。三件事：管理平台（地面+阶梯+高台），管理摄像机（跟随+震屏），管理生成（敌人队列）。
 * 还维护游戏流程状态机（菜单→玩→暂停→死）。
 */

#include "level_api.h"
#include "../core/core_api.h"
#include <string.h>

// ---- 平台数据 ----
static PlatformData g_platforms[MAX_PLATFORMS];
static int g_platformCount = 0;

// ---- 摄像机 ----
static float g_scrollX = 0.0f;       // 当前滚动偏移
static float g_cameraX = 0.0f;       // 摄像机X（平滑后的值）
static float g_shakeIntensity = 0.0f; // 震屏强度（px）
static float g_shakeTimer = 0.0f;     // 震屏剩余时间（秒）

// ---- 流程状态 ----
static GameState g_state = GAME_STATE_MENU;
static GameUI    g_ui    = UI_MENU;
static bool      g_paused = false;

// ---- 关卡信息 ----
static int  g_levelId = 0;
static int  g_totalKills = 0;

// ---- 敌人生成 ----
static float g_spawnTimer = 0.0f;

// 生成请求队列：level产出坐标，main.c消费
static SpawnRequest g_spawnQueue[8];
static int g_spawnQueueLen = 0;

// ============ 生命周期 ============

void Level_Init(void)
{
    memset(g_platforms, 0, sizeof(g_platforms));
    g_platformCount = 0;
    g_scrollX = 0;
    g_cameraX = 0;
    g_shakeIntensity = 0;
    g_shakeTimer = 0;
    g_state = GAME_STATE_MENU;
    g_ui = UI_MENU;
    g_paused = false;
    g_levelId = 0;
    g_totalKills = 0;
    g_spawnTimer = 0;
    g_spawnQueueLen = 0;
}

void Level_Shutdown(void) {}

void Level_Load(int levelId)
{
    g_levelId = levelId;
    memset(g_platforms, 0, sizeof(g_platforms));
    g_platformCount = 0;

    // 1. 底面（覆盖整张地图宽度）
    g_platforms[g_platformCount++] = (PlatformData){
        0, SCREEN_H - 10, MAP_TOTAL, 20, true, true
    };

    // 2. 阶梯式平台
    float spacing = 300.0f;  // 每隔300px左右一个平台
    for (int i = 1; i < MAP_COLS / 5 && g_platformCount < MAX_PLATFORMS; i++) {
        // 位置带点随机偏移，不要看起来太整齐
        float x = i * spacing + Util_Random(0, 80);
        float y = SCREEN_H - 100.0f - (i % 3) * 80.0f;  // 高度循环变化
        float w = 120.0f + Util_Random(0, 80);
        g_platforms[g_platformCount++] = (PlatformData){x, y, w, 20, true, true};

        // 3. 每隔一个加个高平台
        if (i % 2 == 0 && g_platformCount < MAX_PLATFORMS) {
            g_platforms[g_platformCount++] = (PlatformData){
                x + 50 + Util_Random(0, 50), y - 100, 100, 20, true, true
            };
        }
    }

    // 重置关卡统计
    g_totalKills = 0;
    g_spawnTimer = 0;

    // 进入游戏状态
    g_state = GAME_STATE_PLAYING;
    g_ui = UI_GAME;
    g_paused = false;
}

// ============ 每帧更新 ============

void Level_Update(float playerX, float dt)
{
    // ---- 1. 摄像机跟随 ----
    // 目标位置：玩家在屏幕中央
    float target = playerX - SCREEN_W / 2.0f;
    // 限制在地图范围内
    float maxCamera = MAP_TOTAL - SCREEN_W;
    if (maxCamera < 0) maxCamera = 0;
    target = Util_Clamp(target, 0, maxCamera);

    // 平滑移动（lerp系数0.15 = 有点惯性感觉）
    g_cameraX = Util_Lerp(g_cameraX, target, 0.15f);
    g_scrollX = g_cameraX;

    // ---- 2. 震屏衰减 ----
    if (g_shakeTimer > 0) {
        g_shakeTimer -= dt;
        if (g_shakeTimer <= 0)
            g_shakeIntensity = 0;  // 震完了
    }

    // ---- 3. 敌人生成 ----
    if (g_state == GAME_STATE_PLAYING && !g_paused) {
        g_spawnTimer += dt;
        if (g_spawnTimer >= 2.0f) {  // 每2秒生成一批
            g_spawnTimer = 0;

            // 玩家前方生成
            float sx = playerX + SCREEN_W + Util_Random(100, 500);
            if (sx < MAP_TOTAL - 100 && g_spawnQueueLen < 8) {
                // 70%普通 25%装甲 5%Boss
                EnemyType t;
                int r = Util_Random(0, 99);
                if (r < 70)       t = ENEMY_BASIC;
                else if (r < 95)  t = ENEMY_ARMORED;
                else              t = ENEMY_BOSS;

                g_spawnQueue[g_spawnQueueLen++] = (SpawnRequest){
                    sx,
                    SCREEN_H - 100 - Util_Random(0, 3) * 50,  // 随机高度
                    t
                };
            }

            // 玩家后方也生成（防止地图太空）
            sx = playerX - SCREEN_W - Util_Random(100, 500);
            if (sx > 100 && g_spawnQueueLen < 8) {
                int r = Util_Random(0, 99);
                EnemyType t;
                if (r < 70)       t = ENEMY_BASIC;
                else if (r < 95)  t = ENEMY_ARMORED;
                else              t = ENEMY_BOSS;

                g_spawnQueue[g_spawnQueueLen++] = (SpawnRequest){
                    sx,
                    SCREEN_H - 100 - Util_Random(0, 3) * 50,
                    t
                };
            }
        }
    }
}

// ============ 平台查询 ============

int Level_GetPlatforms(PlatformData* out, int maxCount)
{
    int n = g_platformCount < maxCount ? g_platformCount : maxCount;
    memcpy(out, g_platforms, n * sizeof(PlatformData));
    return n;
}

float Level_GetGroundHeight(float x)
{
    // 默认地面高度
    float ground = SCREEN_H - 10;

    // 检查X坐标处有没有更高的平台
    for (int i = 0; i < g_platformCount; i++) {
        const PlatformData* p = &g_platforms[i];
        if (p->active && p->solid && x >= p->x && x <= p->x + p->w) {
            if (p->y < ground)
                ground = p->y;  // 更高的平台覆盖默认地面
        }
    }

    return ground;
}

// ============ 摄像机 ============

float Level_GetScrollX(void)
{
    // 基础偏移 + 震屏随机抖动
    float shake = 0;
    if (g_shakeTimer > 0) {
        shake = Util_Random(-(int)g_shakeIntensity, (int)g_shakeIntensity);
    }
    return g_scrollX + shake;
}

void Level_Shake(float intensity, float duration)
{
    g_shakeIntensity = intensity;
    g_shakeTimer = duration;
}

// ============ 流程状态机 ============

GameState Level_State(void)          { return g_state; }
GameUI    Level_UI(void)             { return g_ui; }
void      Level_SetState(GameState s) { g_state = s; }

void Level_StartGame(void)
{
    Level_Load(1);  // 默认第一关
}

void Level_Pause(void)
{
    g_paused = true;
    g_ui = UI_PAUSE;
}

void Level_Resume(void)
{
    g_paused = false;
    g_ui = UI_GAME;
}

void Level_GameOver(void)
{
    g_state = GAME_STATE_GAMEOVER;
    g_ui = UI_GAMEOVER;
}

bool Level_IsPaused(void) { return g_paused; }

// ============ 敌人生成队列 ============

int Level_PollSpawns(SpawnRequest* out, int maxCount)
{
    int n = g_spawnQueueLen < maxCount ? g_spawnQueueLen : maxCount;
    memcpy(out, g_spawnQueue, n * sizeof(SpawnRequest));

    // 被取走的从队列里移除（前面移走，后面的往前挪）
    if (n > 0) {
        memmove(g_spawnQueue, g_spawnQueue + n,
                (g_spawnQueueLen - n) * sizeof(SpawnRequest));
        g_spawnQueueLen -= n;
    }

    return n;
}

int  Level_CurrentId(void)  { return g_levelId; }
int  Level_TotalKills(void) { return g_totalKills; }
void Level_AddKill(void)    { g_totalKills++; }
