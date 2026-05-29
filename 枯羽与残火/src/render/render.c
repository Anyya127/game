/**
 * render.c — 渲染/表现模块实现
 *
 * 周芷若。三块东西：纹理管理、精灵绘制、音频播放。
 * 资源路径自动搜索，所以不用手动配路径。
 * 别人只要把数据给我，我负责把它画到屏幕上。
 */

#include "render_api.h"
#include "../core/core_api.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

// ============ 纹理资源管理 ============

// 把所有纹理放在一个枚举里，方便用数组管理
typedef enum {
    TEX_BG_MENU,
    TEX_BG_GAME,
    TEX_PLAYER_IDLE, TEX_PLAYER_RUN, TEX_PLAYER_JUMP,
    TEX_PLAYER_SHOOT, TEX_PLAYER_HURT,
    TEX_ENEMY_BASIC, TEX_ENEMY_ARMORED, TEX_ENEMY_BOSS,
    TEX_BULLET_PLAYER, TEX_BULLET_ENEMY,
    TEX_TILE,
    TEX_COUNT  // 总数，用来声明数组长度
} TexId;

static Texture2D g_tex[TEX_COUNT] = {0};
static char     g_resPath[256] = {0};  // resources 目录的完整路径

// 每个纹理对应的文件名（相对于 resources/）
static const char* g_texFiles[TEX_COUNT] = {
    [TEX_BG_MENU]       = "backgrounds/menu_bg.png",
    [TEX_BG_GAME]       = "backgrounds/game_bg.jpg",
    [TEX_PLAYER_IDLE]   = "player/idle.png",
    [TEX_PLAYER_RUN]    = "player/run.png",
    [TEX_PLAYER_JUMP]   = "player/jump.png",
    [TEX_PLAYER_SHOOT]  = "player/shoot.png",
    [TEX_PLAYER_HURT]   = "player/hurt.png",
    [TEX_ENEMY_BASIC]   = "enemies/basic.png",
    [TEX_ENEMY_ARMORED] = "enemies/armored.png",
    [TEX_ENEMY_BOSS]    = "enemies/boss.png",
    [TEX_BULLET_PLAYER] = "effects/bullet_player.png",
    [TEX_BULLET_ENEMY]  = "effects/bullet_enemy.png",
    [TEX_TILE]          = "map/tiles.png",
};

// ============ 资源路径自动搜索 ============
// 因为CMake构建目录、MSVC构建目录、源码目录都可能运行，
// 所以要按顺序尝试几个可能的位置

static bool dirExists(const char* path)
{
    struct stat info;
    return stat(path, &info) == 0 && (info.st_mode & S_IFDIR);
}

static void findResourcePath(char* out, int maxLen)
{
    const char* appDir = GetApplicationDirectory();  // raylib提供的函数

    // 按顺序尝试：同目录 > 上一级 > 上两级 > 直接在appDir下
    const char* cands[] = {"resources", "../resources", "../../resources", ""};
    for (size_t i = 0; i < sizeof(cands)/sizeof(cands[0]); i++) {
        if (cands[i][0])
            snprintf(out, maxLen, "%s/%s", appDir, cands[i]);
        else
            snprintf(out, maxLen, "%s/resources", appDir);

        if (dirExists(out)) return;  // 找到了就用这个
    }

    // 都找不到就默认用第一个
    snprintf(out, maxLen, "%s/resources", appDir);
}

// ============ 纹理加载/卸载 ============

static Texture2D loadTex(const char* filename)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", g_resPath, filename);
    return LoadTexture(path);
}

static void loadAllTextures(void)
{
    for (int i = 0; i < TEX_COUNT; i++) {
        g_tex[i] = loadTex(g_texFiles[i]);
    }
}

static void unloadAllTextures(void)
{
    for (int i = 0; i < TEX_COUNT; i++) {
        if (g_tex[i].id != 0) {
            UnloadTexture(g_tex[i]);
            g_tex[i].id = 0;
        }
    }
}

// ============ 核心绘制函数 ============
// 所有精灵绘制最终都调用这个
// 翻转原理：source.width 设为负数，DrawTexturePro 会自动水平翻转

static void drawSprite(Texture2D tex, float x, float y, float w, float h,
                        bool flip, Color tint)
{
    if (tex.id == 0) {
        // 纹理没加载成功，画个灰块占位
        DrawRectangle((int)x, (int)y, (int)w, (int)h, GRAY);
        return;
    }

    // 假设贴图的每帧是正方形（帧宽 = 纹理高度）
    float fw = (float)tex.height;
    Rectangle src = {0, 0, flip ? -fw : fw, (float)tex.height};
    Rectangle dst = {x, y, w, h};
    DrawTexturePro(tex, src, dst, (Vector2){0, 0}, 0.0f, tint);
}

// ============ 音频管理 ============

typedef enum { BGM_MENU, BGM_GAME, BGM_COUNT } BgmId;

static Music g_bgm[BGM_COUNT];
static BgmId g_currentBgm = BGM_MENU;
static bool  g_bgmPaused = false;
static bool  g_audioInit = false;

static const char* g_bgmFiles[BGM_COUNT] = {
    [BGM_MENU] = "audio/bgm/menu.mp3",
    [BGM_GAME] = "audio/bgm/gameplay.mp3",
};

static void audioInit(void)
{
    InitAudioDevice();
    for (int i = 0; i < BGM_COUNT; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", g_resPath, g_bgmFiles[i]);
        g_bgm[i] = LoadMusicStream(path);  // 流式加载，适合长音频
    }
    g_audioInit = IsAudioDeviceReady();
}

static void audioShutdown(void)
{
    for (int i = 0; i < BGM_COUNT; i++)
        UnloadMusicStream(g_bgm[i]);
    if (g_audioInit) CloseAudioDevice();
}

// ============ 公开接口实现 ============

void Render_Init(const char* resourceDir)
{
    if (resourceDir) {
        strncpy(g_resPath, resourceDir, 255);
    } else {
        findResourcePath(g_resPath, 256);
    }

    loadAllTextures();
    audioInit();
}

void Render_Shutdown(void)
{
    unloadAllTextures();
    audioShutdown();
}

// 每帧调用，驱动流式音乐播放
void Render_UpdateAudio(void)
{
    if (!g_audioInit) return;
    if (g_currentBgm >= 0 && !g_bgmPaused) {
        UpdateMusicStream(g_bgm[g_currentBgm]);
    }
}

// ============ 精灵绘制 ============
// 每个实体类型一个绘制函数，内部根据状态选贴图

static TexId playerTexFromState(PlayerState s)
{
    switch (s) {
    case PLAYER_RUNNING:  return TEX_PLAYER_RUN;
    case PLAYER_JUMPING:  return TEX_PLAYER_JUMP;
    case PLAYER_SHOOTING: return TEX_PLAYER_SHOOT;
    case PLAYER_DYING:
    case PLAYER_DEAD:     return TEX_PLAYER_HURT;
    default:              return TEX_PLAYER_IDLE;
    }
}

void Render_Player(PlayerData* data, float scrollX)
{
    if (!data || data->state == PLAYER_DEAD) return;

    // 无敌闪烁：每10帧闪一次（奇数帧不画）
    if (data->invincible && ((int)(data->invTimer * 60) % 10 < 5)) return;

    // 世界坐标转屏幕坐标（减去摄像机偏移）
    float sx = data->pos.x - scrollX;

    TexId tid = playerTexFromState(data->state);
    drawSprite(g_tex[tid], sx, data->pos.y, data->w, data->h,
               data->facingRight, WHITE);

    // 顶上画血条
    int barW = (int)data->w, barH = 4;
    int bx = (int)sx, by = (int)data->pos.y - 8;
    DrawRectangle(bx, by, barW, barH, DARKGRAY);  // 底条
    float hpPct = (float)data->hp / data->maxHp;
    Color hpCol = hpPct > 0.5f ? GREEN : hpPct > 0.25f ? YELLOW : RED;
    DrawRectangle(bx, by, (int)(barW * hpPct), barH, hpCol);  // 当前血量条
}

static TexId enemyTexFromType(EnemyType t)
{
    switch (t) {
    case ENEMY_ARMORED: return TEX_ENEMY_ARMORED;
    case ENEMY_BOSS:    return TEX_ENEMY_BOSS;
    default:            return TEX_ENEMY_BASIC;
    }
}

void Render_Enemy(const EnemyData* data, float scrollX)
{
    if (!data || !data->active || data->state == AI_DEAD) return;

    float sx = data->pos.x - scrollX;

    // 不在屏幕范围内的不画（视锥裁剪）
    if (sx + data->w < 0 || sx > SCREEN_W) return;

    TexId tid = enemyTexFromType(data->type);
    drawSprite(g_tex[tid], sx, data->pos.y, data->w, data->h,
               data->facingRight, WHITE);

    // 装甲兵和Boss画血条（杂兵不画，太多了乱）
    if (data->type != ENEMY_BASIC) {
        int barW = (int)data->w, barH = 4;
        int bx = (int)sx, by = (int)data->pos.y - 8;
        DrawRectangle(bx, by, barW, barH, DARKGRAY);
        float hpPct = (float)data->hp / data->maxHp;
        Color hpCol = hpPct > 0.5f ? GREEN : hpPct > 0.25f ? YELLOW : RED;
        DrawRectangle(bx, by, (int)(barW * hpPct), barH, hpCol);
    }
}

void Render_Bullet(const BulletData* data, float scrollX)
{
    if (!data || !data->active) return;

    float sx = data->pos.x - scrollX;

    TexId tid = (data->type == BULLET_PLAYER) ? TEX_BULLET_PLAYER : TEX_BULLET_ENEMY;
    drawSprite(g_tex[tid], sx, data->pos.y, data->w, data->h, false, WHITE);
}

void Render_Platform(const PlatformData* data, float scrollX)
{
    if (!data || !data->active) return;

    float sx = data->x - scrollX;
    if (sx + data->w < 0 || sx > SCREEN_W) return;  // 视锥裁剪

    Texture2D tile = g_tex[TEX_TILE];
    if (tile.id == 0) {
        // 没瓦片贴图，用纯色方块代替
        DrawRectangle((int)sx, (int)data->y, (int)data->w, (int)data->h, BROWN);
        DrawRectangleLines((int)sx, (int)data->y, (int)data->w, (int)data->h, BLACK);
        if (data->solid) {
            DrawRectangle((int)sx, (int)data->y, (int)data->w, 5, GREEN);  // 顶部绿线
        }
        return;
    }

    // 瓦片平铺：把平台分成小格子，逐个贴
    float ts = (float)tile.height;  // 瓦片是正方形的
    for (float tx = 0; tx < data->w; tx += ts) {
        for (float ty = 0; ty < data->h; ty += ts) {
            float dw = (tx + ts > data->w) ? data->w - tx : ts;
            float dh = (ty + ts > data->h) ? data->h - ty : ts;
            Rectangle src = {0, 0, dw, dh};
            Rectangle dst = {sx + tx, data->y + ty, dw, dh};
            DrawTexturePro(tile, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }
}

// ============ 背景 ============

void Render_GameBackground(float cameraX)
{
    Texture2D bg = g_tex[TEX_BG_GAME];
    if (bg.id == 0) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, SKYBLUE);
        return;
    }

    // 背景缩放适配屏幕高度，按0.3倍速度滚动（视差效果）
    float scale = SCREEN_H / (float)bg.height;
    float sw = bg.width * scale;
    float px = fmod(-cameraX * 0.3f, sw);  // 0.3 = 远景移动速度
    if (px > 0) px -= sw;

    for (float x = px; x < SCREEN_W; x += sw) {
        DrawTexturePro(bg,
            (Rectangle){0, 0, bg.width, bg.height},
            (Rectangle){x, 0, sw, SCREEN_H},
            (Vector2){0, 0}, 0.0f, WHITE);
    }
}

void Render_MenuBackground(void)
{
    Texture2D bg = g_tex[TEX_BG_MENU];
    if (bg.id == 0) {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, DARKBLUE);
        return;
    }
    DrawTexturePro(bg,
        (Rectangle){0, 0, bg.width, bg.height},
        (Rectangle){0, 0, SCREEN_W, SCREEN_H},
        (Vector2){0, 0}, 0.0f, WHITE);
}

// ============ UI ============

void Render_UI(int hp, int maxHp, int lives, int score, int level, int kills)
{
    // 左上角状态面板
    DrawRectangle(10, 10, 200, 80, Fade(BLACK, 0.5f));  // 半透明黑底

    char buf[64];

    snprintf(buf, sizeof(buf), "HP: %d/%d", hp, maxHp);
    DrawText(buf, 20, 15, 20, hp > maxHp / 2 ? GREEN : RED);

    snprintf(buf, sizeof(buf), "LIVES: %d", lives);
    DrawText(buf, 20, 40, 20, YELLOW);

    snprintf(buf, sizeof(buf), "SCORE: %d", score);
    DrawText(buf, 20, 65, 20, BLUE);

    // 右上角关卡信息和击杀数
    snprintf(buf, sizeof(buf), "LEVEL %d", level);
    DrawText(buf, SCREEN_W - MeasureText(buf, 25) - 10, 10, 25, MAGENTA);

    snprintf(buf, sizeof(buf), "KILLS: %d", kills);
    DrawText(buf, SCREEN_W - MeasureText(buf, 20) - 10, 40, 20, ORANGE);
}

void Render_Menu(void)
{
    const char* title = "Loss and Tenacity";
    int ts = 60;
    DrawText(title, (SCREEN_W - MeasureText(title, ts)) / 2, 150, ts, YELLOW);

    DrawText("CONTROLS:", SCREEN_W / 2 - 80, 300, 25, GREEN);
    DrawText("Move: Arrow Keys / WASD",    SCREEN_W / 2 - 120, 335, 20, WHITE);
    DrawText("Jump: SPACE / W / UP",       SCREEN_W / 2 - 120, 360, 20, WHITE);
    DrawText("Shoot: Z / J / Left Mouse",  SCREEN_W / 2 - 140, 385, 20, WHITE);
    DrawText("Pause: P / ESC",             SCREEN_W / 2 - 100, 410, 20, WHITE);

    // 闪烁的 "Press ENTER"
    const char* hint = "Press ENTER to Start";
    int hs = 35;
    Color hc = (fmod(GetTime(), 1.0f) < 0.5f) ? YELLOW : WHITE;
    DrawText(hint, (SCREEN_W - MeasureText(hint, hs)) / 2, 500, hs, hc);
}

void Render_Pause(void)
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.7f));  // 半透明遮罩

    const char* t = "PAUSED";
    int ts = 60;
    DrawText(t, (SCREEN_W - MeasureText(t, ts)) / 2, 200, ts, YELLOW);

    t = "Press P to Resume";
    ts = 30;
    DrawText(t, (SCREEN_W - MeasureText(t, ts)) / 2, 300, ts, WHITE);
}

void Render_GameOver(int score, int kills)
{
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.8f));

    const char* t = "GAME OVER";
    int ts = 70;
    DrawText(t, (SCREEN_W - MeasureText(t, ts)) / 2, 150, ts, RED);

    char buf[128];
    snprintf(buf, sizeof(buf), "FINAL SCORE: %d", score);
    ts = 40;
    DrawText(buf, (SCREEN_W - MeasureText(buf, ts)) / 2, 250, ts, YELLOW);

    snprintf(buf, sizeof(buf), "TOTAL KILLS: %d", kills);
    DrawText(buf, (SCREEN_W - MeasureText(buf, ts)) / 2, 300, ts, ORANGE);

    t = "Press R to Restart";
    ts = 35;
    DrawText(t, (SCREEN_W - MeasureText(t, ts)) / 2, 400, ts, GREEN);
}

// ============ 表现效果 ============

void VFX_ProcessEvents(const Event* events, int count, float dt)
{
    (void)dt;
    for (int i = 0; i < count; i++) {
        const Event* ev = &events[i];
        switch (ev->type) {
        case EVT_HIT:
            // 打中敌人：微微一震
            VFX_Screenshake(3.0f, 0.1f);
            break;
        case EVT_PARRY:
            // 弹反成功：大震动 + 停顿
            VFX_Screenshake(8.0f, 0.2f);
            VFX_Hitstop(4);
            break;
        case EVT_KILL:
            // 击杀：中等震动 + 稍长停顿
            VFX_Screenshake(5.0f, 0.15f);
            VFX_Hitstop(6);
            break;
        case EVT_PLAYER_HURT:
            // 玩家受伤：提醒性震动
            VFX_Screenshake(4.0f, 0.15f);
            break;
        case EVT_PLAYER_DEAD:
            // 玩家死亡：重震
            VFX_Screenshake(10.0f, 0.5f);
            break;
        default:
            break;
        }
    }
}

void VFX_Screenshake(float intensity, float duration)
{
    // 震屏通过level模块的摄像机抖动实现
    extern void Level_Shake(float, float);
    Level_Shake(intensity, duration);
}

void VFX_Hitstop(int frames)
{
    // hitstop 会让画面暂停几帧，增强打击感
    // TODO: 需要帧循环配合实现
    (void)frames;
}

void VFX_CameraFollow(float targetX)
{
    // 摄像机跟随在level模块
    (void)targetX;
}

// ============ 音频快捷接口 ============

void Audio_PlayBGM(int bgmType, float volume)
{
    if (!g_audioInit || bgmType < 0 || bgmType >= BGM_COUNT) return;

    // 停掉当前BGM，播新的
    if (g_currentBgm >= 0)
        StopMusicStream(g_bgm[g_currentBgm]);

    g_currentBgm = (BgmId)bgmType;
    SetMusicVolume(g_bgm[g_currentBgm], volume);
    PlayMusicStream(g_bgm[g_currentBgm]);
    g_bgmPaused = false;
}

void Audio_PauseBGM(void)
{
    if (g_currentBgm >= 0) {
        PauseMusicStream(g_bgm[g_currentBgm]);
        g_bgmPaused = true;
    }
}

void Audio_ResumeBGM(void)
{
    if (g_currentBgm >= 0) {
        ResumeMusicStream(g_bgm[g_currentBgm]);
        g_bgmPaused = false;
    }
}
