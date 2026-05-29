/**
 * combat.c — 战斗模块实现
 *
 * 陈雨桐。两件事：管子弹，做碰撞。
 * 碰撞检测结果是"事件"，存队列里。main.c 消费事件来扣血，render 消费事件来做特效。
 */

#include "combat_api.h"
#include "../player/player_api.h"
#include "../enemy/enemy_api.h"
#include "../core/core_api.h"
#include <string.h>
#include <stdlib.h>

// ---- 子弹池 ----
static BulletData g_bullets[MAX_BULLETS];  // 30个槽位
static int g_bulletCount = 0;
static int g_nextBulletId = 500;  // 子弹ID从500开始，跟玩家(1)和敌人(100+)区分

// ---- 事件队列 ----
static Event g_events[MAX_EVENTS];  // 本帧发生的所有战斗事件
static int g_eventCount = 0;
static int g_combo = 0;  // 连击计数器

void Combat_Init(void)
{
    memset(g_bullets, 0, sizeof(g_bullets));
    memset(g_events, 0, sizeof(g_events));
    g_bulletCount = 0;
    g_eventCount = 0;
    g_combo = 0;
    g_nextBulletId = 500;
}

void Combat_Shutdown(void)
{
    Bullet_ClearAll();
}

// ============ 子弹管理 ============

Entity Bullet_Create(float x, float y, float vx, float vy,
                      BulletType type, int damage)
{
    // 找空槽位
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) {
            BulletData* b = &g_bullets[i];
            b->id     = g_nextBulletId++;
            b->pos    = (Vec2){x, y};
            b->vel    = (Vec2){vx, vy};
            b->damage = damage;
            b->type   = type;
            b->active = true;
            // 玩家子弹长方形，敌人子弹方块
            b->w = (type == BULLET_PLAYER) ? 36.0f : 30.0f;
            b->h = (type == BULLET_PLAYER) ? 18.0f : 30.0f;
            g_bulletCount++;
            return b->id;
        }
    }
    return -1;  // 满了
}

void Bullet_UpdateAll(float dt)
{
    (void)dt;  // 暂时不用dt，子弹速度恒定的
    for (int i = 0; i < MAX_BULLETS; i++) {
        BulletData* b = &g_bullets[i];
        if (!b->active) continue;

        // 按速度移动
        b->pos.x += b->vel.x;
        b->pos.y += b->vel.y;

        // 飞出屏幕/地图就销毁
        if (b->pos.x < -50 || b->pos.x > MAP_TOTAL + 50 ||
            b->pos.y < -100 || b->pos.y > SCREEN_H + 100) {
            Bullet_Destroy(b->id);
        }
    }
}

void Bullet_Destroy(Entity e)
{
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (g_bullets[i].active && g_bullets[i].id == e) {
            memset(&g_bullets[i], 0, sizeof(BulletData));
            g_bulletCount--;
            return;
        }
    }
}

void Bullet_ClearAll(void)
{
    memset(g_bullets, 0, sizeof(g_bullets));
    g_bulletCount = 0;
}

int Bullet_Count(void) { return g_bulletCount; }

void Bullet_GetAll(BulletData* out, int maxCount)
{
    int idx = 0;
    for (int i = 0; i < MAX_BULLETS && idx < maxCount; i++) {
        if (g_bullets[i].active)
            out[idx++] = g_bullets[i];
    }
}

// ============ AABB碰撞检测 ============
// 经典矩形重叠判断：两个矩形在X和Y轴上都有重叠才算碰撞

static bool rectsOverlap(float x1, float y1, float w1, float h1,
                          float x2, float y2, float w2, float h2)
{
    return x1 < x2 + w2 && x1 + w1 > x2 &&
           y1 < y2 + h2 && y1 + h1 > y2;
}

// ============ 发射事件 ============

static void emit(CombatEvent type, Entity src, Entity tgt,
                  float x, float y, int damage)
{
    if (g_eventCount >= MAX_EVENTS) return;  // 队列满了就丢弃

    g_events[g_eventCount++] = (Event){
        .type = type,
        .source = src,
        .target = tgt,
        .x = x, .y = y,
        .damage = damage,
        .combo = g_combo
    };
}

// ============ 主判定：遍历所有子弹和敌人做碰撞检测 ============

void Combat_Update(Entity playerEntity,
                   const EnemyData* enemies, int enemyCount,
                   const BulletData* bullets, int bulletCount,
                   float dt)
{
    (void)dt;
    g_eventCount = 0;  // 清空上帧的事件

    // 玩家死了就不做判定了
    PlayerData player = Player_GetData(playerEntity);
    if (player.state == PLAYER_DEAD) return;

    Rect playerBox = {player.pos.x, player.pos.y, player.w, player.h};

    // ---- 遍历所有子弹 ----
    for (int i = 0; i < bulletCount; i++) {
        const BulletData* b = &bullets[i];
        if (!b->active) continue;

        Rect bulletBox = {b->pos.x, b->pos.y, b->w, b->h};

        if (b->type == BULLET_PLAYER) {
            // 玩家子弹：检查是否打到了任何一个敌人
            for (int j = 0; j < enemyCount; j++) {
                const EnemyData* e = &enemies[j];
                if (!e->active || e->state >= AI_DYING) continue;

                Rect enemyBox = {e->pos.x, e->pos.y, e->w, e->h};
                if (rectsOverlap(bulletBox.x, bulletBox.y, bulletBox.w, bulletBox.h,
                                 enemyBox.x, enemyBox.y, enemyBox.w, enemyBox.h)) {
                    // 打中了！
                    g_combo++;
                    emit(EVT_HIT, playerEntity, e->id,
                         e->pos.x + e->w / 2, e->pos.y + e->h / 2, b->damage);
                    Bullet_Destroy(b->id);  // 子弹消失
                    break;  // 这颗子弹只打一个敌人
                }
            }
        } else {
            // 敌人子弹：检查是否打到了玩家
            if (!player.invincible &&
                rectsOverlap(bulletBox.x, bulletBox.y, bulletBox.w, bulletBox.h,
                             playerBox.x, playerBox.y, playerBox.w, playerBox.h)) {
                emit(EVT_PLAYER_HURT, b->id, playerEntity,
                     player.pos.x + player.w / 2, player.pos.y + player.h / 2,
                     b->damage);
                Bullet_Destroy(b->id);
            }
        }
    }

    // ---- 接触伤害：敌人碰到玩家 ----
    if (!player.invincible) {
        for (int j = 0; j < enemyCount; j++) {
            const EnemyData* e = &enemies[j];
            if (!e->active || e->state >= AI_DYING) continue;

            Rect enemyBox = {e->pos.x, e->pos.y, e->w, e->h};
            if (rectsOverlap(playerBox.x, playerBox.y, playerBox.w, playerBox.h,
                             enemyBox.x, enemyBox.y, enemyBox.w, enemyBox.h)) {
                emit(EVT_PLAYER_HURT, e->id, playerEntity,
                     player.pos.x + player.w / 2, player.pos.y + player.h / 2,
                     e->contactDamage);
            }
        }
    }
}

// ============ 事件消费 ============

int Combat_PollEvents(Event* out, int maxEvents)
{
    int n = g_eventCount < maxEvents ? g_eventCount : maxEvents;
    memcpy(out, g_events, n * sizeof(Event));
    return n;
}

void Combat_ClearEvents(void)
{
    g_eventCount = 0;
}

int Combat_ComboCount(void)
{
    return g_combo;
}
