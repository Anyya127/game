/**
 * main.c — 游戏主循环
 *
 * 张小凡（队友6）维护。这里是全组的"装配车间"：
 * 所有模块的 Init/Update/Draw 在这里按顺序调。
 * 不写具体逻辑，只做调度——数据的搬运工。
 */

#include "player/player_api.h"
#include "enemy/enemy_api.h"
#include "combat/combat_api.h"
#include "render/render_api.h"
#include "level/level_api.h"
#include "core/core_api.h"
#include "raylib.h"
#include <stdlib.h>
#include <time.h>

// ============ 战斗事件处理 ============
// combat产出的事件的消费者之一（另一个是render做特效）
// 这里的职责：扣血、加分、判断死亡

static void processCombatEvents(Entity player)
{
    Event evts[32];
    int n = Combat_PollEvents(evts, 32);

    // 先让render处理特效（震屏/hitstop）
    VFX_ProcessEvents(evts, n, Core_DeltaTime());

    // 然后处理逻辑后果
    for (int i = 0; i < n; i++) {
        const Event* ev = &evts[i];
        switch (ev->type) {
        case EVT_HIT:
            // 玩家子弹打中敌人 → 敌人扣血
            Enemy_TakeDamage(ev->target, ev->damage);
            if (Enemy_IsDead(ev->target)) {
                // 死了加分
                int pts = Enemy_GetScoreValue(ev->target);
                Player_AddScore(player, pts);
                Level_AddKill();
            }
            break;

        case EVT_PLAYER_HURT:
            // 玩家被子弹/接触伤到了
            Player_TakeDamage(player, ev->damage);
            if (Player_IsDead(player)) {
                Level_GameOver();
            }
            break;

        default:
            break;
        }
    }

    Combat_ClearEvents();  // 消费完了，清空队列
}

// ============ 敌人生成 ============
// level模块产出SpawnRequest，这里调用Enemy_Spawn创建

static void processSpawns(void)
{
    SpawnRequest reqs[8];
    int n = Level_PollSpawns(reqs, 8);
    for (int i = 0; i < n; i++) {
        Enemy_Spawn(reqs[i].x, reqs[i].y, reqs[i].type);
    }
}

// ============ 主函数 ============

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    srand((unsigned int)time(NULL));  // 随机种子

    // ---- 全模块初始化 ----
    Core_InitWindow(SCREEN_W, SCREEN_H, "Enchanted Warrior");
    Render_Init(NULL);   // NULL = 自动搜索resources路径
    Player_Init();
    Enemy_Init();
    Combat_Init();
    Level_Init();

    // 菜单BGM开始播放
    Audio_PlayBGM(0, 1.0f);

    // ---- 游戏主循环 ----
    while (!Core_ShouldClose()) {
        float dt = Core_DeltaTime();
        PlayerInput in = Core_PollInput();
        Render_UpdateAudio();  // 流式音乐需要每帧更新

        GameState state = Level_State();

        // ========== 菜单状态 ==========
        if (state == GAME_STATE_MENU) {
            if (IsKeyPressed(KEY_ENTER)) {
                // 按回车开始游戏
                Level_StartGame();
                Audio_PlayBGM(1, 1.0f);   // 切到游戏BGM
                Player_Create(100, SCREEN_H - 200);
            }
            Core_BeginFrame();
            Render_MenuBackground();
            Render_Menu();
            Core_EndFrame();
            continue;
        }

        // ========== 死亡状态 ==========
        if (state == GAME_STATE_GAMEOVER) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
                // 按R或回车重新开始
                Enemy_ClearAll();
                Bullet_ClearAll();
                Level_StartGame();
                Audio_PlayBGM(1, 1.0f);
                Player_Create(100, SCREEN_H - 200);
            }
            Entity player = Player_GetEntity();
            PlayerData pd = Player_GetData(player);
            Core_BeginFrame();
            Render_GameOver(pd.score, Level_TotalKills());
            Core_EndFrame();
            continue;
        }

        // ========== 游戏中 ==========
        // 暂停切换
        if (in.pause) {
            if (Level_IsPaused()) {
                Level_Resume();
                Audio_ResumeBGM();
            } else {
                Level_Pause();
                Audio_PauseBGM();
            }
        }

        // ---- 逻辑更新（暂停时跳过） ----
        if (!Level_IsPaused()) {
            Entity player = Player_GetEntity();
            if (player == -1) goto render;  // 还没创建玩家

            // 1. 关卡更新（摄像机 + 敌人生成计时）
            PlayerData pd = Player_GetData(player);
            Level_Update(pd.pos.x, dt);

            // 2. 拿平台数据
            PlatformData platforms[MAX_PLATFORMS];
            int platCount = Level_GetPlatforms(platforms, MAX_PLATFORMS);

            // 3. 玩家更新（输入 → 移动 → 碰撞）
            pd = Player_Update(player, in, dt, platforms, platCount);

            // 4. 玩家射击 → 创建子弹
            if (pd.wantsShoot) {
                float bx = pd.facingRight ? pd.pos.x + pd.w : pd.pos.x - 12;
                float by = pd.pos.y + pd.h / 3.0f;
                float vx = pd.facingRight ? PLAYER_BULLET_SPD : -PLAYER_BULLET_SPD;
                Bullet_Create(bx, by, vx, 0, BULLET_PLAYER, PLAYER_BULLET_DMG);
            }

            // 5. 敌人生成（消费level队列）
            processSpawns();

            // 6. 敌人AI更新
            Enemy_UpdateAll(pd.pos.x, pd.pos.y, dt, platforms, platCount);

            // 7. 敌人射击 → 创建子弹
            {
                EnemyData enemies[MAX_ENEMIES];
                int ec = Enemy_Count();
                Enemy_GetAll(enemies, MAX_ENEMIES);
                for (int i = 0; i < ec; i++) {
                    if (enemies[i].wantsShoot && enemies[i].active
                        && enemies[i].state != AI_DEAD) {
                        float bx = enemies[i].facingRight
                            ? enemies[i].pos.x + enemies[i].w
                            : enemies[i].pos.x - 10;
                        float by = enemies[i].pos.y + enemies[i].h / 3.0f;
                        float vx = enemies[i].facingRight
                            ? ENEMY_BULLET_SPD : -ENEMY_BULLET_SPD;
                        Bullet_Create(bx, by, vx, 0, BULLET_ENEMY, ENEMY_BULLET_DMG);
                    }
                }
            }

            // 8. 子弹移动
            Bullet_UpdateAll(dt);

            // 9. 战斗判定（碰撞检测 + 事件发射）
            {
                EnemyData enemies[MAX_ENEMIES];
                int ec = Enemy_Count();
                Enemy_GetAll(enemies, MAX_ENEMIES);

                BulletData bullets[MAX_BULLETS];
                int bc = Bullet_Count();
                Bullet_GetAll(bullets, MAX_BULLETS);

                Combat_Update(player, enemies, ec, bullets, bc, dt);
            }

            // 10. 处理战斗事件（扣血/加分/死亡判定）
            processCombatEvents(player);

            // 检查是否死人
            if (Player_IsDead(player)) {
                Level_GameOver();
            }
        }

render:
        // ---- 渲染 ----
        float scrollX = Level_GetScrollX();  // 当前摄像机偏移（含震屏）

        Core_BeginFrame();
        GameState gs = Level_State();

        if (gs == GAME_STATE_PLAYING) {
            Entity player = Player_GetEntity();
            PlayerData pd = Player_GetData(player);

            // 背景（视差滚动）
            Render_GameBackground(pd.pos.x);

            // 平台
            {
                PlatformData platforms[MAX_PLATFORMS];
                int pc = Level_GetPlatforms(platforms, MAX_PLATFORMS);
                for (int i = 0; i < pc; i++)
                    Render_Platform(&platforms[i], scrollX);
            }

            // 敌人
            {
                EnemyData enemies[MAX_ENEMIES];
                int ec = Enemy_Count();
                Enemy_GetAll(enemies, MAX_ENEMIES);
                for (int i = 0; i < ec; i++)
                    Render_Enemy(&enemies[i], scrollX);
            }

            // 子弹
            {
                BulletData bullets[MAX_BULLETS];
                int bc = Bullet_Count();
                Bullet_GetAll(bullets, MAX_BULLETS);
                for (int i = 0; i < bc; i++)
                    Render_Bullet(&bullets[i], scrollX);
            }

            // 玩家
            Render_Player(&pd, scrollX);

            // UI
            Render_UI(pd.hp, pd.maxHp, pd.lives,
                      pd.score, Level_CurrentId(), Level_TotalKills());

            // 暂停遮罩（覆盖在最上层）
            if (Level_IsPaused()) Render_Pause();
        }

        Core_EndFrame();
    }

    // ---- 清理所有模块 ----
    Player_Shutdown();
    Enemy_Shutdown();
    Combat_Shutdown();
    Render_Shutdown();
    Level_Shutdown();
    Core_CloseWindow();

    return 0;
}
