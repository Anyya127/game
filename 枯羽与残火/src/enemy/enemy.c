/**
 * enemy.c — 敌人模块实现
 *
 * 王思远。核心是一个状态机控制的AI + 一个数组池管理所有敌人。
 * 敌人存在 g_enemies[MAX_ENEMIES] 数组里，用 active 标志位复用槽位。
 */

#include "enemy_api.h"
#include "../core/core_api.h"
#include <string.h>

// 敌人池：20个槽位，用完就不能再创建了
static EnemyData g_enemies[MAX_ENEMIES];
static int g_count = 0;       // 当前活着的敌人数量
static int g_nextId = 100;    // 自增ID，从100开始（跟玩家ID区分）

void Enemy_Init(void)
{
    memset(g_enemies, 0, sizeof(g_enemies));
    g_count = 0;
    g_nextId = 100;
}

void Enemy_Shutdown(void)
{
    Enemy_ClearAll();
}

// 分配一个新ID
static Entity allocateId(void)
{
    return g_nextId++;
}

// ============ 创建敌人 ============

Entity Enemy_Create(float x, float y, EnemyType type)
{
    // 遍历找空槽位
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            EnemyData* e = &g_enemies[i];
            e->id     = allocateId();
            e->pos    = (Vec2){x, y};
            e->vel    = (Vec2){0, 0};
            e->type   = type;
            e->active = true;
            e->facingRight = false;    // 初始朝左（通常玩家在右边过来）
            e->attackCooldown = 0.0f;
            e->patrolTimer = 0.0f;
            e->state = AI_PATROL;      // 一出生就开始巡逻
            e->wantsShoot = false;

            // 不同类型有不同属性
            switch (type) {
            case ENEMY_BASIC:
                e->hp = e->maxHp = BASIC_ENEMY_HP;      // 50血
                e->w = 40; e->h = 60;
                e->detectRange = 300;                   // 300px外就能发现玩家
                e->scoreValue = 100;
                e->contactDamage = 10;
                break;
            case ENEMY_ARMORED:
                e->hp = e->maxHp = ARMORED_ENEMY_HP;    // 100血
                e->w = 50; e->h = 70;                   // 比杂兵大一圈
                e->detectRange = 250;                   // 近视一些
                e->scoreValue = 200;
                e->contactDamage = 20;
                break;
            case ENEMY_BOSS:
                e->hp = e->maxHp = BOSS_ENEMY_HP;       // 500血
                e->w = 100; e->h = 120;                 // 很大
                e->detectRange = 400;                   // 视力好
                e->scoreValue = 500;
                e->contactDamage = 30;
                break;
            }

            g_count++;
            return e->id;
        }
    }
    // 20个槽位全满了，创建失败
    return -1;
}

// ============ AI 状态机 ============
//
// 状态转换图：
//   IDLE(发呆2s) → PATROL(左右走)
//   PATROL ←→ CHASE(发现玩家就追)
//   CHASE ←→ ATTACK(追到了就开枪)
//   任何状态 → DYING/DEAD(血量为0)

static void updateAI(EnemyData* e, float playerX, float playerY, float dt)
{
    // 每帧默认不想射击，只有ATTACK状态成功冷却才设true
    e->wantsShoot = false;

    // 死了就别动了
    if (e->state >= AI_DYING) return;

    // 计算跟玩家的距离和方向
    float dx = playerX - e->pos.x;
    float dist = Util_Distance(e->pos.x, e->pos.y, playerX, playerY);
    // 始终面朝玩家
    e->facingRight = (dx > 0);

    switch (e->state) {

    case AI_IDLE:
        // 发呆，站2秒后去巡逻
        e->vel.x = 0;
        e->patrolTimer += dt;
        if (e->patrolTimer > 2.0f) {
            e->state = AI_PATROL;
            e->patrolTimer = 0;
        }
        break;

    case AI_PATROL:
        // 巡逻：走1秒停1秒，然后转身
        e->patrolTimer += dt;
        if (e->patrolTimer < 1.0f) {
            e->vel.x = e->facingRight ? 2.0f : -2.0f;  // 慢速移动
        } else {
            e->vel.x = 0;  // 停下
            if (e->patrolTimer > 2.0f) {
                e->patrolTimer = 0;
                e->facingRight = !e->facingRight;  // 转身
            }
        }
        // 玩家进入检测范围 → 追
        if (dist < e->detectRange) e->state = AI_CHASE;
        break;

    case AI_CHASE:
        // 追击：朝玩家方向高速移动
        e->vel.x = (dx > 0) ? 8.0f : -8.0f;
        // 够近了就开火
        if (dist < e->detectRange * 0.7f) e->state = AI_ATTACK;
        // 追丢了（玩家跑出1.5倍检测距离）→ 回去巡逻
        if (dist > e->detectRange * 1.5f) e->state = AI_PATROL;
        break;

    case AI_ATTACK:
        // 站桩射击
        e->vel.x = 0;
        if (e->attackCooldown <= 0.0f) {
            e->attackCooldown = 2.0f;       // 2秒一发
            e->wantsShoot = true;           // 通知外面：我要开枪
        }
        // 玩家跑远了 → 继续追
        if (dist > e->detectRange * 0.6f) e->state = AI_CHASE;
        break;

    default:
        e->vel.x = 0;
        break;
    }
}

// ============ 物理 ============

static void applyPhysics(EnemyData* e)
{
    // 跟玩家一样的重力+摩擦力
    e->vel.y += GRAVITY;
    e->vel.x *= FRICTION;
    e->pos.x += e->vel.x;
    e->pos.y += e->vel.y;

    // 站地面（简化处理，没做平台碰撞）
    float ground = SCREEN_H - 10;
    if (e->pos.y + e->h >= ground) {
        e->pos.y = ground - e->h;
        e->vel.y = 0;
    }
}

// ============ 主更新 ============

void Enemy_UpdateAll(float playerX, float playerY, float dt,
                      const PlatformData* platforms, int platformCount)
{
    // 暂时没用到平台数据（敌人只在底面走），保留接口
    (void)platforms;
    (void)platformCount;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        EnemyData* e = &g_enemies[i];
        if (!e->active || e->state == AI_DEAD) continue;

        // 冷却递减
        if (e->attackCooldown > 0.0f) e->attackCooldown -= dt;

        // AI决策 → 物理移动
        updateAI(e, playerX, playerY, dt);
        applyPhysics(e);
    }
}

// ============ 战斗接口 ============

void Enemy_TakeDamage(Entity e, int damage)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active || g_enemies[i].id != e) continue;

        g_enemies[i].hp -= damage;
        if (g_enemies[i].hp <= 0) {
            // 死了：血量归零，标记为死亡状态，释放槽位
            g_enemies[i].hp = 0;
            g_enemies[i].state = AI_DEAD;
            g_enemies[i].active = false;
            g_count--;  // 活着的数量减一
        }
        return;
    }
}

bool Enemy_IsDead(Entity e)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].id == e)
            return g_enemies[i].state == AI_DEAD;
    }
    return true;  // 找不到就当作已死
}

int Enemy_GetScoreValue(Entity e)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].id == e)
            return g_enemies[i].scoreValue;
    }
    return 0;
}

// ============ 查询 ============

EnemyData Enemy_GetData(Entity e)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].id == e)
            return g_enemies[i];
    }
    return (EnemyData){0};  // 没找到返回全零
}

int Enemy_Count(void) { return g_count; }

void Enemy_GetAll(EnemyData* out, int maxCount)
{
    int idx = 0;
    for (int i = 0; i < MAX_ENEMIES && idx < maxCount; i++) {
        if (g_enemies[i].active && g_enemies[i].state != AI_DEAD) {
            out[idx++] = g_enemies[i];  // 拷贝出去，外面改不到内部数据
        }
    }
}

void Enemy_ClearAll(void)
{
    memset(g_enemies, 0, sizeof(g_enemies));
    g_count = 0;
}

void Enemy_Spawn(float x, float y, EnemyType type)
{
    Enemy_Create(x, y, type);
}
