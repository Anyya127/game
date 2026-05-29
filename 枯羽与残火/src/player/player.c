/**
 * player.c — 玩家模块实现
 *
 * 李铭。核心思路就一个：游戏只有一个玩家，所以用全局变量存就够了。
 * Player_Update 是一帧逻辑的入口，按顺序：输入 → 物理 → 碰撞 → 边界。
 */

#include "player_api.h"
#include "../core/core_api.h"

// 只有一个玩家，所以一个全局变量搞定
static PlayerData g_player;
static bool g_initialized = false;

void Player_Init(void)
{
    g_player = (PlayerData){0};
    g_player.id = -1;  // -1 表示还没创建
    g_initialized = true;
}

void Player_Shutdown(void)
{
    g_initialized = false;
}

Entity Player_Create(float x, float y)
{
    // 玩家ID固定为1，因为只有一个玩家
    g_player.id = 1;
    g_player.pos = (Vec2){x, y};
    g_player.vel = (Vec2){0, 0};
    g_player.hp = PLAYER_HP;
    g_player.maxHp = PLAYER_HP;
    g_player.lives = PLAYER_LIVES;
    g_player.score = 0;
    g_player.w = PLAYER_W;
    g_player.h = PLAYER_H;
    g_player.grounded = false;
    g_player.facingRight = true;  // 默认朝右
    g_player.state = PLAYER_IDLE;
    g_player.shootCooldown = 0.0f;
    g_player.invTimer = 0.0f;
    g_player.invincible = false;
    g_player.wantsShoot = false;

    return g_player.id;
}

void Player_Destroy(Entity e)
{
    (void)e;
    g_player.id = -1;
}

// ============ 这是最核心的函数，一帧里玩家所有的行为都在这里 ============

PlayerData Player_Update(Entity e, PlayerInput in, float dt,
                          const PlatformData* platforms, int platformCount)
{
    (void)e;
    // 死了就别动了
    if (g_player.state == PLAYER_DEAD) return g_player;

    // ---- 1. 冷却计时器递减 ----
    if (g_player.shootCooldown > 0.0f)
        g_player.shootCooldown -= dt;

    if (g_player.invincible && g_player.invTimer > 0.0f) {
        g_player.invTimer -= dt;
        if (g_player.invTimer <= 0.0f)
            g_player.invincible = false;  // 无敌时间到了
    }

    // ---- 2. 处理输入，改速度和朝向 ----
    if (in.moveLeft) {
        g_player.vel.x = -PLAYER_SPEED;
        g_player.facingRight = false;
        if (g_player.grounded) g_player.state = PLAYER_RUNNING;
    } else if (in.moveRight) {
        g_player.vel.x = PLAYER_SPEED;
        g_player.facingRight = true;
        if (g_player.grounded) g_player.state = PLAYER_RUNNING;
    } else {
        // 没按方向键就停下来
        g_player.vel.x = 0.0f;
        if (g_player.grounded) g_player.state = PLAYER_IDLE;
    }

    // 跳跃：必须站在地上才能跳
    if (in.jump && g_player.grounded) {
        g_player.vel.y = PLAYER_JUMP;  // 负数 = 往上弹
        g_player.grounded = false;
        g_player.state = PLAYER_JUMPING;
    }

    // 射击：设置wantsShoot标志，main.c看到后会调Bullet_Create
    g_player.wantsShoot = false;
    if (in.shoot && g_player.shootCooldown <= 0.0f) {
        g_player.shootCooldown = 0.25f;  // 4发/秒
        g_player.state = PLAYER_SHOOTING;
        g_player.wantsShoot = true;
    } else if (!in.shoot && g_player.state == PLAYER_SHOOTING) {
        g_player.state = PLAYER_IDLE;  // 松开射击键就恢复
    }

    // ---- 3. 物理 ----
    // 重力：每帧往下加速
    g_player.vel.y += GRAVITY;
    // 摩擦力：水平速度每帧衰减
    g_player.vel.x *= FRICTION;

    // 位移
    g_player.pos.x += g_player.vel.x;
    g_player.pos.y += g_player.vel.y;

    // ---- 4. 平台碰撞 ----
    // 思路：检查脚底是否踩到了哪个平台
    g_player.grounded = false;
    for (int i = 0; i < platformCount; i++) {
        const PlatformData* p = &platforms[i];
        if (!p->active || !p->solid) continue;

        float myBottom  = g_player.pos.y + g_player.h;
        float myRight   = g_player.pos.x + g_player.w;
        float platRight = p->x + p->w;
        float platBot   = p->y + p->h;

        // 先看水平方向有没有重叠
        bool xOverlap = g_player.pos.x < platRight && myRight > p->x;
        if (!xOverlap) continue;

        // 往下落，脚底穿过平台顶 → 踩上去了
        if (g_player.vel.y >= 0 &&
            myBottom >= p->y &&
            myBottom - g_player.vel.y <= p->y + 10) {
            g_player.pos.y = p->y - g_player.h;  // 贴在平台顶上
            g_player.vel.y = 0.0f;
            g_player.grounded = true;
        }
        // 往上跳，头顶撞到平台底 → 弹回来
        else if (g_player.vel.y < 0 &&
                 g_player.pos.y <= platBot &&
                 g_player.pos.y - g_player.vel.y >= platBot - 5) {
            g_player.pos.y = platBot;  // 贴在平台底下
            g_player.vel.y = 0.0f;
        }
    }

    // 如果没站在任何平台上，检查是否站在地面（屏幕底部）
    if (!g_player.grounded) {
        float ground = SCREEN_H - 10;  // 地面线
        if (g_player.pos.y + g_player.h >= ground && g_player.vel.y >= 0) {
            g_player.pos.y = ground - g_player.h;
            g_player.vel.y = 0.0f;
            g_player.grounded = true;
        }
    }

    // ---- 5. 边界限制 ----
    // X轴不能跑出地图
    if (g_player.pos.x < 0)
        g_player.pos.x = 0;
    if (g_player.pos.x > MAP_TOTAL - g_player.w)
        g_player.pos.x = MAP_TOTAL - g_player.w;

    // Y轴掉出屏幕 = 摔死
    if (g_player.pos.y > SCREEN_H + 200) {
        g_player.hp = 0;
        g_player.state = PLAYER_DEAD;
    }

    return g_player;
}

// ============ 下面的查询函数都只是读数据，不改状态 ============

PlayerData Player_GetData(Entity e) {
    (void)e;
    return g_player;
}

Entity Player_GetEntity(void) {
    return g_player.id;
}

Rect Player_GetBounds(Entity e) {
    (void)e;
    return (Rect){g_player.pos.x, g_player.pos.y, g_player.w, g_player.h};
}

bool Player_IsDead(Entity e) {
    (void)e;
    return g_player.state == PLAYER_DEAD;
}

// ============ 战斗系统通过这些函数修改玩家状态 ============

void Player_TakeDamage(Entity e, int damage)
{
    (void)e;
    // 无敌或已经死了就忽略
    if (g_player.invincible || g_player.state == PLAYER_DEAD) return;

    g_player.hp -= damage;
    // 受伤后短时间无敌，防止连续被扣血
    g_player.invincible = true;
    g_player.invTimer = 1.0f;  // 1秒无敌

    if (g_player.hp <= 0) {
        g_player.hp = 0;
        g_player.lives--;
        if (g_player.lives <= 0) {
            // 没命了，真死了
            g_player.state = PLAYER_DEAD;
        } else {
            // 还有命，回满血重生
            g_player.hp = g_player.maxHp;
            g_player.state = PLAYER_DYING;
        }
    }
}

void Player_AddScore(Entity e, int points) {
    (void)e;
    g_player.score += points;
}

void Player_Reset(Entity e, float x, float y) {
    (void)e;
    g_player.pos = (Vec2){x, y};
    g_player.vel = (Vec2){0, 0};
    g_player.hp = g_player.maxHp;
    g_player.grounded = false;
    g_player.state = PLAYER_IDLE;
    g_player.invincible = false;
    g_player.invTimer = 0.0f;
}
