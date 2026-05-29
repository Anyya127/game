# Enchanted Warrior — 设计文档

## 游戏概述

横版侧视高速动作游戏。核心玩法：弹反启动连击，近战输出，枪械追击，弹反后瞬移到敌人背后。

- 视角：横版侧视
- 战斗节奏：高速连击
- 核心操作：弹反/格挡
- 主角：刀 + 枪，近远双修
- 敌人：杂兵 + 精英 + Boss 混搭

---

## 设计哲学

1. **死得快，重试更快** — 死亡到重生不超过半秒，无加载画面，鼓励冒险和精确操作
2. **所有反馈都要过量** — 弹反成功不只是弹反成功：震屏 + hitstop + 粒子 + 音效，玩家不看 HUD 就知道自己打得好不好
3. **数据定义行为，代码定义规则** — 招式表、敌人属性、连击窗口全在 JSON 数据文件里，代码只写系统逻辑
4. **系统要少，做到极致** — 不做装备、技能树、RPG 数值。只有弹反 + 连击 + 瞬移 + 枪械追击

---

## 项目结构

```
enchanted_warrior/
├── CMakeLists.txt
├── assets/
│   ├── animations/          # 精灵帧动画数据 (JSON)
│   ├── textures/            # 贴图资源 (PNG)
│   ├── audio/               # 音效和音乐 (WAV/OGG)
│   ├── moves/               # 招式定义 (JSON)
│   └── levels/              # 关卡配置 (JSON)
│
├── engine/                  # 底层 C++ ECS 引擎（组长负责）
│   ├── core/
│   │   ├── ECS.h            # EnTT 别名 + 辅助
│   │   ├── TimeManager.h/cpp    # 全局时间缩放、hitstop
│   │   ├── InputBuffer.h/cpp    # 输入环形缓冲区
│   │   ├── ResourceManager.h/cpp# 纹理/音频/招式加载
│   │   └── Math.h/cpp           # 向量、AABB、工具函数
│   ├── components/
│   │   ├── Transform.h      # Position、Rotation、Scale、ZLayer
│   │   ├── Physics.h        # Velocity、Gravity、GroundSensor
│   │   ├── Combat.h         # Health、Damage、Faction、Hitbox
│   │   ├── Move.h           # MoveState、MoveDef、帧计数
│   │   ├── Input.h          # InputBuffer、PlayerInput
│   │   ├── Render.h         # Sprite、Animation、Particle
│   │   └── Tags.h           # Player、Enemy、Projectile 等 Tag
│   ├── systems/
│   │   ├── PhysicsSystem.h/cpp   # 重力、移动、地面碰撞
│   │   ├── MoveSystem.h/cpp      # 招式状态机，帧推进
│   │   ├── ParrySystem.h/cpp     # 弹反窗口判定
│   │   ├── CombatSystem.h/cpp    # 伤害结算、死亡
│   │   ├── AISystem.h/cpp        # 敌人行为决策
│   │   ├── InputSystem.h/cpp     # 输入采集、缓冲写入
│   │   ├── AnimationSystem.h/cpp # 精灵帧驱动
│   │   ├── FeedbackSystem.h/cpp  # 震屏、hitstop、粒子管理
│   │   └── RenderSystem.h/cpp    # 统一渲染管线
│   ├── factories/
│   │   ├── PlayerFactory.h/cpp   # 从 moves JSON 创建玩家实体
│   │   └── EnemyFactory.h/cpp    # 从 moves JSON 创建敌人实体
│   └── game/
│       ├── GameLoop.h/cpp        # 初始化、帧循环、System 调度
│       ├── LevelLoader.h/cpp     # 加载 levels JSON，生成实体
│       └── GameState.h/cpp       # 状态切换（菜单/游戏/暂停/死亡/通关）
│
├── api/                      # C 接口胶水层（组长负责）
│   ├── api.h                 # 纯 C 头文件，队友唯一需要 #include 的底层头文件
│   └── api.cpp               # C 函数 → 内部 C++ 的桥接实现
│
└── src/                      # 队友编写的上层逻辑（C）
    ├── combat/
    │   ├── combat.h
    │   └── combat.c          # 战斗判定：弹反检测、连击计数、伤害结算
    ├── move/
    │   ├── move.h
    │   └── move.c            # 招式执行：招式选择、取消规则、动画同步
    ├── ai/
    │   ├── ai.h
    │   └── ai.c              # 敌人 AI：巡逻、追击、攻击、撤退
    ├── vfx/
    │   ├── vfx.h
    │   └── vfx.c             # 表现层：粒子、震屏、hitstop、音效触发
    ├── level/
    │   ├── level.h
    │   └── level.c           # 关卡逻辑：敌人编排、检查点、关卡切换
    └── main.c                # 主入口，游戏循环装配
```

---

## 依赖关系

```
engine/  ← 底层 C++ ECS 引擎，不依赖任何上层模块
  ↑
api/     ← C 接口胶水层，仅依赖 engine/
  ↑
src/     ← 队友模块，仅依赖 api/api.h
  │
  ├── combat/   → 被 vfx/ 通过回调订阅
  ├── move/     → 被 combat/ 和 ai/ 调用
  ├── ai/       → 调用 move/ 发起攻击
  ├── vfx/      → 订阅 combat 的事件回调，触发表现
  └── level/    → 调用 ai/、combat/、move/ 配置关卡
```

编译产物：
- `libengine.a` — engine/ + api/ 编译的静态库
- `libcombat.a`, `libmove.a`, `libai.a`, `libvfx.a`, `liblevel.a` — 各模块独立静态库
- `main.c` → `enchanted_warrior` — 最终可执行文件

---

## C 接口设计 (api/api.h)

### 设计原则

- 所有类型对队友透明：`World*` 是不透明指针，`Entity` 是整数
- 纯 C 结构体，无 C++ 特性
- 每个底层能力封装为一个 C 函数
- 队友看不到 EnTT、std::vector、任何模板

### 类型定义

```c
// === 不透明句柄 ===
typedef struct World World;
typedef int Entity;
typedef int TextureID;
typedef int SoundID;
typedef int MovesetID;

// === 纯 C 结构体 ===
typedef struct { float x, y; int layer; } Transform;
typedef struct { float vx, vy; float gravity_mult; } Physics;
typedef struct { float current, max; } Health;
typedef struct { float x, y, w, h; } Rect;
typedef struct { float x, y; } Vec2;
```

### 招式数据（只读查询）

```c
typedef struct {
    int    id;
    char   name[32];
    int    startup_frames;
    int    active_frames;
    int    recovery_frames;
    int    parry_window_start;   // 弹反可判定起始帧
    int    parry_window_end;     // 弹反可判定结束帧
    int    cancel_window_start;  // 取消窗口起始帧
    int    cancel_window_end;    // 取消窗口结束帧
    int    cancel_into[16];      // 可取消到的招式 ID 列表
    int    cancel_into_count;
    float  damage;
    float  knockback_x;
    float  knockback_y;
    Rect   hitbox;               // 攻击判定盒（相对自身位置）
    int    animation_id;
    int    sound_id;
} MoveDef;
```

### 函数分组

#### 生命周期
```c
World*    world_create(int screen_w, int screen_h, const char* title);
void      world_destroy(World* w);
int       world_should_close(World* w);
```

#### 实体
```c
Entity    world_spawn(World* w);
void      world_despawn(World* w, Entity e);
int       entity_valid(World* w, Entity e);
```

#### 组件 — Tag
```c
void      tag_player(World* w, Entity e);
void      tag_enemy(World* w, Entity e);
void      tag_projectile(World* w, Entity e);
int       tag_is_player(World* w, Entity e);
int       tag_is_enemy(World* w, Entity e);
```

#### 组件 — Transform
```c
void      trans_set(World* w, Entity e, float x, float y, int layer);
Transform trans_get(World* w, Entity e);
void      trans_move(World* w, Entity e, float dx, float dy);
```

#### 组件 — Physics
```c
void      phys_set(World* w, Entity e, float vx, float vy, float grav);
Physics   phys_get(World* w, Entity e);
void      phys_set_vel(World* w, Entity e, float vx, float vy);
```

#### 组件 — Health
```c
void      hp_set(World* w, Entity e, float current, float max);
Health    hp_get(World* w, Entity e);
void      hp_damage(World* w, Entity e, float amount);
void      hp_heal(World* w, Entity e, float amount);
```

#### 组件 — Hitbox
```c
void      hitbox_set(World* w, Entity e, float x, float y, float w_, float h_);
Rect      hitbox_get(World* w, Entity e);
```

#### 组件 — Move
```c
void      move_set_moveset(World* w, Entity e, int moveset_id);
int       move_get_current(World* w, Entity e);
int       move_get_phase(World* w, Entity e);  // 0=startup,1=active,2=recovery,-1=idle
int       move_get_frame(World* w, Entity e);
Rect      move_get_attack_box(World* w, Entity e);  // 当前帧攻击判定盒（世界坐标）
int       move_in_parry_window(World* w, Entity e);
int       move_can_cancel(World* w, Entity e, int target_move_id);
```

#### 输入
```c
void      input_update(World* w);
int       input_down(World* w, int key);
int       input_pressed(World* w, int key);   // 仅本帧刚按下
int       input_released(World* w, int key);  // 仅本帧刚松开
int       input_hold_frames(World* w, int key);// 按住持续帧数
```

#### 碰撞查询
```c
int       query_overlap(World* w, Rect box, Entity* out, int max_out);
int       query_ground(World* w, Entity e);  // 返回平台 Y 坐标，-1 表示未着地
```

#### 招式数据查询
```c
int       res_moveset_count(World* w, int moveset_id);
MoveDef  res_moveset_get(World* w, int moveset_id, int index);
int       res_moveset_find(World* w, int moveset_id, const char* name);
```

#### 时间
```c
float     time_delta(World* w);
void      time_set_scale(World* w, float scale);
float     time_get_scale(World* w);
void      time_hitstop(World* w, int frames);
```

#### 渲染
```c
void      render_begin(World* w);
void      render_end(World* w);
void      render_sprite(World* w, int tex_id, int frame, float x, float y,
                        int flip_x, float sx, float sy);
void      render_rect(World* w, float x, float y, float w_, float h_,
                      int r, int g, int b, int a);
void      render_text(World* w, const char* text, float x, float y, int size,
                      int r, int g, int b, int a);
void      camera_set_pos(World* w, float x, float y);
void      camera_get_pos(World* w, float* x, float* y);
void      camera_shake(World* w, float intensity, float duration);
```

#### 资源
```c
int       res_texture(World* w, const char* path);
int       res_sound(World* w, const char* path);
int       res_moveset(World* w, const char* json_path);
void      audio_play(World* w, int sound_id, float vol, float pitch);
void      audio_stop(World* w, int sound_id);
```

#### 粒子
```c
void      particle_spawn(World* w, float x, float y, float vx, float vy,
                         int r, int g, int b, int a, float life, float size);
void      particle_burst(World* w, float x, float y, int count,
                         int r, int g, int b, int a, float life, float size);
```

---

## 队友模块职责与接口

### 模块1: combat（战斗判定）

**职责**：
- 遍历所有实体，检测攻击判定盒与受击盒的碰撞
- 弹反输入与敌方招式 `parry_window` 的帧匹配
- 伤害结算、连击计数器管理
- 死亡触发

**暴露给其他模块**：
```c
// combat.h
void combat_init(World* w);
void combat_update(World* w);

// 我方发起攻击
void combat_player_attack(World* w, int move_id);
// 我方弹反
void combat_player_parry(World* w);
// 获取当前连击数
int  combat_combo_count(World* w);

// 事件回调（表现层订阅）
typedef void (*CombatOnHit)(World* w, Entity a, Entity b, float dmg, float x, float y);
typedef void (*CombatOnParry)(World* w, Entity a, Entity b, float x, float y, int combo);
typedef void (*CombatOnKill)(World* w, Entity a, Entity b, float x, float y);

void combat_subscribe_hit(World* w, CombatOnHit cb);
void combat_subscribe_parry(World* w, CombatOnParry cb);
void combat_subscribe_kill(World* w, CombatOnKill cb);
```

### 模块2: move（招式系统）

**职责**：
- 按招式数据推进帧状态机
- 判断当前攻击阶段（startup/active/recovery）
- 招式取消规则检查
- 动画帧同步

```c
// move.h
void move_init(World* w);
void move_update(World* w);

// 让实体执行招式
void move_execute(World* w, Entity e, int move_id);
// 当前招式信息查询
int  move_current_id(World* w, Entity e);
int  move_current_phase(World* w, Entity e);
int  move_current_frame(World* w, Entity e);
Rect move_attack_box(World* w, Entity e);
int  move_in_parry_window(World* w, Entity e);
int  move_can_cancel(World* w, Entity e, int move_id);
```

### 模块3: ai（敌人 AI）

**职责**：
- AI 状态机（Idle → Patrol → Chase → Attack → Retreat）
- 根据距离、血量、玩家状态切换状态
- 攻击时选择合适的招式
- 不同敌人的行为参数差异

```c
// ai.h
void ai_init(World* w);
void ai_update(World* w);

// 给单个敌人配置 AI
void ai_configure(World* w, Entity e,
                  float patrol_l, float patrol_r,
                  float chase_range, float attack_range,
                  float retreat_hp_ratio);

// 查询敌人当前状态
int  ai_get_state(World* w, Entity e); // 0=idle,1=patrol,2=chase,3=attack,4=retreat
```

### 模块4: vfx（表现）

**职责**：
- 粒子系统生命周期管理
- 震屏衰减
- Hitstop 计时器
- 连击数字显示
- 音效触发策略（弹反层数不同用不同音效变体）
- 弹反闪光、瞬移残影

```c
// vfx.h
void vfx_init(World* w);
void vfx_update(World* w);

// 手动触发（队友调用）
void vfx_spark(World* w, float x, float y, int count);
void vfx_hitstop(World* w, int frames);
void vfx_screenshake(World* w, float intensity, float duration);
void vfx_combo_number(World* w, float x, float y, int combo_count);
void vfx_teleport_flash(World* w, float x, float y);
void vfx_screen_flash(World* w, int r, int g, int b, int a, int frames);

// 事件处理（在 update 中自动响应）
// vfx 内部调用 combat_subscribe_* 注册，队友不需要手动订阅
```

### 模块5: level（关卡 + 流程）

**职责**：
- 从 assets/levels/*.json 加载关卡配置
- 按关卡数据创建敌人实体，调用 `ai_configure` 配置
- 管理检查点
- 游戏流程状态机：菜单 → 游戏 → 暂停 → 死亡 → 通关
- 死亡后重生逻辑

```c
// level.h
void level_init(World* w);
void level_update(World* w);

void level_load(World* w, int level_id);
void level_unload(World* w);
void level_restart(World* w);
void level_restart_checkpoint(World* w);
int  level_current(World* w);

// 流程状态
int  flow_get_state(World* w); // 0=menu,1=playing,2=paused,3=dead,4=victory
void flow_set_state(World* w, int state);
```

### main.c — 主循环

```c
int main(void) {
    World* w = world_create(1280, 720, "Enchanted Warrior");

    // 初始化所有模块
    combat_init(w);
    move_init(w);
    ai_init(w);
    vfx_init(w);
    level_init(w);

    level_load(w, 1); // 加载第一关

    while (!world_should_close(w)) {
        input_update(w);

        // 逻辑更新
        level_update(w);   // 流程状态机
        move_update(w);    // 招式推进
        ai_update(w);      // 敌人决策
        combat_update(w);  // 战斗判定
        vfx_update(w);     // 表现更新

        // 渲染
        render_begin(w);
        render_end(w);
    }

    world_destroy(w);
    return 0;
}
```

---

## 数据格式

### 招式定义 JSON (assets/moves/sword.json)

```json
{
  "moveset": "player_sword",
  "moves": [
    {
      "id": 0,
      "name": "light_1",
      "startup_frames": 4,
      "active_frames": 3,
      "recovery_frames": 5,
      "parry_window_start": 1,
      "parry_window_end": 3,
      "cancel_window_start": 3,
      "cancel_window_end": 5,
      "cancel_into": [1, 4],
      "damage": 15.0,
      "knockback_x": 3.0,
      "knockback_y": 0.0,
      "hitbox": { "x": 30, "y": -10, "w": 40, "h": 30 },
      "animation_id": 0,
      "sound_id": 0
    },
    {
      "id": 1,
      "name": "light_2",
      "startup_frames": 3,
      "active_frames": 3,
      "recovery_frames": 6,
      "parry_window_start": 1,
      "parry_window_end": 3,
      "cancel_window_start": 3,
      "cancel_window_end": 6,
      "cancel_into": [2, 4],
      "damage": 18.0,
      "knockback_x": 3.5,
      "knockback_y": 0.0,
      "hitbox": { "x": 35, "y": -10, "w": 45, "h": 30 },
      "animation_id": 1,
      "sound_id": 0
    },
    {
      "id": 2,
      "name": "light_3",
      "startup_frames": 5,
      "active_frames": 4,
      "recovery_frames": 8,
      "parry_window_start": 1,
      "parry_window_end": 4,
      "cancel_window_start": 4,
      "cancel_window_end": 8,
      "cancel_into": [4],
      "damage": 25.0,
      "knockback_x": 6.0,
      "knockback_y": 2.0,
      "hitbox": { "x": 35, "y": -15, "w": 55, "h": 35 },
      "animation_id": 2,
      "sound_id": 1
    },
    {
      "id": 4,
      "name": "parry",
      "startup_frames": 1,
      "active_frames": 4,
      "recovery_frames": 2,
      "parry_window_start": 1,
      "parry_window_end": 4,
      "cancel_window_start": 0,
      "cancel_window_end": 0,
      "cancel_into": [],
      "damage": 0.0,
      "knockback_x": 0.0,
      "knockback_y": 0.0,
      "hitbox": { "x": -20, "y": -20, "w": 40, "h": 40 },
      "animation_id": 10,
      "sound_id": 5
    },
    {
      "id": 5,
      "name": "gun_shot",
      "startup_frames": 2,
      "active_frames": 1,
      "recovery_frames": 3,
      "parry_window_start": 0,
      "parry_window_end": 0,
      "cancel_window_start": 0,
      "cancel_window_end": 0,
      "cancel_into": [0, 1],
      "damage": 5.0,
      "knockback_x": 8.0,
      "knockback_y": 0.0,
      "hitbox": { "x": 40, "y": -5, "w": 200, "h": 10 },
      "animation_id": 20,
      "sound_id": 10
    }
  ]
}
```

### 关卡定义 JSON (assets/levels/level_01.json)

```json
{
  "level_id": 1,
  "name": "Ruined Streets",
  "background_tex": "bg_street",
  "ground_y": 650,
  "platforms": [
    { "x": 200, "y": 500, "w": 300 },
    { "x": 600, "y": 420, "w": 250 },
    { "x": 1000, "y": 350, "w": 300 }
  ],
  "checkpoints": [
    { "x": 100, "y": 600 },
    { "x": 800, "y": 600 },
    { "x": 1600, "y": 600 }
  ],
  "enemies": [
    {
      "type": "grunt",
      "moveset": "enemy_sword",
      "x": 300, "y": 600,
      "hp": 40,
      "ai": { "patrol_l": 250, "patrol_r": 400, "chase_range": 200, "attack_range": 50, "retreat_hp_ratio": 0.2 }
    },
    {
      "type": "elite",
      "moveset": "enemy_shield",
      "x": 700, "y": 600,
      "hp": 100,
      "ai": { "patrol_l": 650, "patrol_r": 800, "chase_range": 300, "attack_range": 60, "retreat_hp_ratio": 0.3 }
    },
    {
      "type": "boss",
      "moveset": "boss_blade",
      "x": 1800, "y": 600,
      "hp": 400,
      "ai": { "patrol_l": 1700, "patrol_r": 2000, "chase_range": 500, "attack_range": 120, "retreat_hp_ratio": 0.0 }
    }
  ],
  "player_spawn": { "x": 100, "y": 600 }
}
```

---

## 帧循环顺序

```
InputSystem      → 采集按键，写入输入缓冲区
AISystem         → 敌人根据距离和状态选择招式
MoveSystem       → 推进招式帧数，驱动攻击判定盒
ParrySystem      → 检测弹反输入是否命中敌方招式窗口
CombatSystem     → 伤害结算、死亡检查、连击更新
PhysicsSystem    → 移动、重力、地面碰撞
FeedbackSystem   → 根据战斗事件触发震屏/hitstop/粒子
AnimationSystem  → 根据招式状态和物理状态更新精灵帧
RenderSystem     → 统一渲染所有实体
```

---

## 关键设计决策

### 输入缓冲

环形缓冲区保存最近 6 帧的按键状态。弹反键按下后不直接检测生效，而是推进缓冲区。
ParrySystem 每帧检查缓冲区中是否有待消费的弹反输入，与敌对攻击的 `parry_window` 比对。
这让"前几帧的按键"也能生效，大幅提升手感。

### 弹反判定

弹反不是"按下去就生效一个盾牌"，而是：
1. 玩家执行 parry 招式，进入 active 阶段
2. 该 active 阶段的帧必须在敌方攻击的 `parry_window` 范围内
3. 且玩家的弹反判定盒与敌方的攻击判定盒有重叠
4. 三个条件同时满足 → 弹反成功

### 招式取消

每个招式定义了自己的 `cancel_window`（哪些帧可以取消）和 `cancel_into`（可以取消到哪些招式）。
轻击连段（light_1 → light_2 → light_3）通过取消链实现，
而不是硬编码的连段表。修改取消规则只需改 JSON。

### 连击追击

弹反成功后敌人进入硬直（标志：`can_cancel=false`一段时间）。
连击计数器仅在这个窗口内递增。连击越高，玩家伤害略有放大。
敌人被击飞到远处时，用枪械招式拉回来（枪械的 knockback_x 是负值处理）。

### 瞬移

弹反成功后生成一个可瞬移标记在敌人身上。
玩家按瞬移键+方向 → 消耗标记，传送到敌人背后。
背后攻击有伤害加成。

---

## 构建系统

CMake，分层编译：

```
engine/  → libengine.a     (C++，编译为静态库)
api/     → 同上（和 engine 一起编译）
src/*    → libcombat.a, libmove.a, libai.a, libvfx.a, liblevel.a
main.c   → 链接上述所有 .a，生成可执行文件
```

依赖库：
- raylib 5.x: 渲染、输入、音频、窗口
- EnTT: ECS 框架（header-only）
- nlohmann/json: JSON 解析（header-only）

---

## 开发顺序

| 阶段 | 内容 | 负责人 |
|------|------|--------|
| P0 | engine/core 骨架（ECS, TimeManager, InputBuffer, ResourceManager, Math） | 组长 |
| P0 | api/api.h + api.cpp 完整 C 接口 | 组长 |
| P0 | components 全部定义 | 组长 |
| P0 | PhysicsSystem + RenderSystem + InputSystem | 组长 |
| P1 | PlayerFactory + EnemyFactory + MoveSystem | 组长 |
| P1 | 示例招式 JSON + 示例关卡 JSON | 组长 |
| P2 | main.c 空循环 + 一个可动的玩家验证 P0/P1 | 组长 |
| P3 | combat 模块（队友1） | 队友 |
| P3 | move 模块（队友2） | 队友 |
| P3 | ai 模块（队友3） | 队友 |
| P3 | vfx 模块（队友4） | 队友 |
| P3 | level 模块（队友5） | 队友 |
| P4 | 集成调试 | 全组 |
| P5 | 美术/音效资源替换 | 全组 |
