/**
 * @file types.h
 * @brief 共享类型定义 — 所有模块唯一共同依赖的头文件
 *
 * 设计原则：
 * - 不依赖任何业务头文件
 * - 只定义"数据形状"，不定义"行为"
 * - 纯 C，无 raylib 业务类型泄漏
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

// ==================== 基础数学类型 ====================
typedef struct { float x, y; } Vec2;
typedef struct { float x, y, w, h; } Rect;

// ==================== 实体 ID（-1 表示无效） ====================
typedef int Entity;

// ==================== 游戏状态枚举 ====================
typedef enum {
    GAME_STATE_MENU,    // 主菜单
    GAME_STATE_PLAYING, // 游戏中
    GAME_STATE_PAUSED,  // 暂停
    GAME_STATE_GAMEOVER // 游戏结束
} GameState;

typedef enum {
    UI_MENU,    // 菜单界面
    UI_GAME,    // 游戏画面
    UI_PAUSE,   // 暂停界面
    UI_GAMEOVER // 死亡界面
} GameUI;

// ==================== 输入状态 ====================
typedef struct {
    bool moveLeft;
    bool moveRight;
    bool jump;
    bool shoot;
    bool parry;     // 弹反
    bool teleport;  // 瞬移
    bool pause;
    bool restart;
} PlayerInput;

// ==================== 玩家数据 ====================
typedef enum {
    PLAYER_IDLE,
    PLAYER_RUNNING,
    PLAYER_JUMPING,
    PLAYER_SHOOTING,
    PLAYER_DYING,
    PLAYER_DEAD
} PlayerState;

typedef struct {
    Entity id;
    Vec2 pos;
    Vec2 vel;
    int hp;
    int maxHp;
    int lives;
    int score;
    float w, h;
    bool grounded;
    bool facingRight;
    PlayerState state;
    float shootCooldown;
    float invTimer;
    bool invincible;
    bool wantsShoot;    // 本帧是否发射子弹（player 设 true，main.c 消费）
} PlayerData;

// ==================== 敌人数据 ====================
typedef enum {
    ENEMY_BASIC,   // 杂兵
    ENEMY_ARMORED, // 装甲兵
    ENEMY_BOSS     // Boss
} EnemyType;

typedef enum {
    AI_IDLE,   // 待机
    AI_PATROL, // 巡逻
    AI_CHASE,  // 追击
    AI_ATTACK, // 攻击
    AI_DYING,  // 死亡中
    AI_DEAD    // 已死亡
} AIState;

typedef struct {
    Entity id;
    Vec2 pos;
    Vec2 vel;
    int hp;
    int maxHp;
    float w, h;
    EnemyType type;
    AIState state;
    bool active;
    bool facingRight;
    float attackCooldown;
    float patrolTimer;
    float detectRange;
    int scoreValue;
    int contactDamage;
    bool wantsShoot;    // 本帧是否发射子弹（AI 进入攻击状态时置位，main.c 消费）
} EnemyData;

// ==================== 子弹数据 ====================
typedef enum {
    BULLET_PLAYER, // 玩家子弹
    BULLET_ENEMY   // 敌人子弹
} BulletType;

typedef struct {
    Entity id;
    Vec2 pos;
    Vec2 vel;
    int damage;
    BulletType type;
    bool active;
    float w, h;
} BulletData;

// ==================== 平台数据 ====================
typedef struct {
    float x, y, w, h;
    bool solid;
    bool active;
} PlatformData;

// ==================== 战斗事件（表现层订阅） ====================
typedef enum {
    EVT_NONE,
    EVT_HIT,         // 攻击命中
    EVT_PARRY,       // 弹反成功
    EVT_KILL,        // 击杀
    EVT_PLAYER_HURT, // 玩家受伤
    EVT_PLAYER_DEAD, // 玩家死亡
} CombatEvent;

typedef struct {
    CombatEvent type;
    Entity source;  // 攻击者
    Entity target;  // 受击者
    float x, y;     // 事件发生位置
    int damage;
    int combo;      // 当前连击数
} Event;

// ==================== 配置常量 ====================

// ---- 窗口 ----
#define SCREEN_W   1280     // 窗口宽度（px）
#define SCREEN_H   720      // 窗口高度（px）
#define TARGET_FPS 60       // 目标帧率

// ---- 玩家 ----
#define PLAYER_HP         100   // 初始血量
#define PLAYER_LIVES      3     // 初始命数
#define PLAYER_W          40    // 碰撞体宽度（px）
#define PLAYER_H          60    // 碰撞体高度（px）
#define PLAYER_SPEED      5.0f  // 水平移速（px/帧）
#define PLAYER_JUMP       -12.0f // 跳跃初速度（负值=向上，px/帧）
#define PLAYER_BULLET_SPD 10.0f // 子弹飞行速度（px/帧）
#define PLAYER_BULLET_DMG 45  // 子弹伤害（足够秒杀杂兵）

// ---- 敌人 ----
#define ENEMY_BULLET_SPD 6.0f  // 敌人子弹速度（px/帧）
#define ENEMY_BULLET_DMG 10    // 敌人子弹伤害

#define BASIC_ENEMY_HP   50    // 杂兵血量
#define ARMORED_ENEMY_HP 100   // 装甲兵血量
#define BOSS_ENEMY_HP    500   // Boss 血量

// ---- 物理 ----
#define GRAVITY  0.5f  // 重力加速度（px/帧²）
#define FRICTION 0.8f  // 水平速度衰减系数（每帧乘此值）

// ---- 地图 ----
#define MAP_TILE  40                   // 瓦片尺寸（px，正方形）
#define MAP_COLS  200                  // 地图横向瓦片数
#define MAP_TOTAL (MAP_TILE * MAP_COLS) // 地图总宽度（8000px）
#define MAX_PLATFORMS 50               // 平台最大数量
#define MAX_ENEMIES   20               // 同时在场敌人上限
#define MAX_BULLETS   (10 + 20)        // 子弹槽位总数（玩家10 + 敌人20）
#define MAX_EVENTS    32               // 每帧最大战斗事件数

#endif // TYPES_H
