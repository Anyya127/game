# 枯羽与残火 (Enchanted Warrior) — 架构文档

## 编译与启动

### Linux

```bash
# 1. 安装系统依赖（仅第一次，需要 sudo）
./scripts/setup.sh

# 2. 配置 CMake
cmake -S . -B build -DBUILD_EXAMPLES=OFF

# 3. 编译
cmake --build build -j$(nproc)

# 4. 运行
./build/EnchantedWarrior
```

### Windows 11

**右键 `scripts/setup_windows.bat`，选择"以管理员身份运行"**。脚本会自动：
1. 安装 winget（Win11 自带，无需操作）
2. 安装 CMake
3. 安装 MinGW-w64（GCC 编译器）
4. 编译生成 `build/EnchantedWarrior.exe`

每一步成功后可能需要重新打开命令行窗口再运行下一步。全自动不需要手动操作。

如果 winget 安装失败，手动准备环境只需三样东西：
- [CMake](https://cmake.org/download/) (≥3.16)
- [MinGW-w64](https://winlibs.com/) (选 UCRT, x86_64, posix 线程模型)
- 把两者的 `bin/` 目录加入系统 PATH

然后运行：
```powershell
cmake -S . -B build -DBUILD_EXAMPLES=OFF -G "MinGW Makefiles"
cmake --build build -j
build\EnchantedWarrior.exe
```

### 依赖说明

`vendor/raylib/` 包含 raylib 5.5 完整源码（不含 examples、games 等非必要文件），离线编译，无需联网下载任何依赖。

## 项目结构

```
枯羽与残火/
├── CMakeLists.txt               # CMake 构建文件
├── ARCHITECTURE.md              # 本文档
├── scripts/setup.sh             # 系统依赖安装脚本（Ubuntu/Debian）
├── vendor/raylib/               # raylib 5.5 源码（离线）
├── resources/                   # 美术与音频资源
│   ├── backgrounds/             # 背景图（menu_bg.png, game_bg.jpg）
│   ├── player/                  # 玩家精灵帧（idle, run, jump, shoot, hurt）
│   ├── enemies/                 # 敌人精灵（basic, armored, boss）
│   ├── effects/                 # 特效（子弹贴图）
│   ├── map/                     # 地图瓦片
│   └── audio/bgm/               # 背景音乐（menu.mp3, gameplay.mp3）
│
└── src/
    ├── types/types.h            # 共享类型定义（所有人依赖）
    │
    ├── player/                  # 模块1：玩家
    │   ├── player_api.h         #   公开接口
    │   ├── player.c             #   实现
    │   └── 说明.md              #   模块架构文档
    │
    ├── enemy/                   # 模块2：敌人AI
    │   ├── enemy_api.h
    │   ├── enemy.c
    │   └── 说明.md
    │
    ├── combat/                  # 模块3：战斗判定
    │   ├── combat_api.h
    │   ├── combat.c
    │   └── 说明.md
    │
    ├── render/                  # 模块4：渲染与表现
    │   ├── render_api.h
    │   ├── render.c
    │   └── 说明.md
    │
    ├── level/                   # 模块5：关卡与流程
    │   ├── level_api.h
    │   ├── level.c
    │   └── 说明.md
    │
    ├── core/                    # 模块6：核心封装
    │   ├── core_api.h
    │   ├── core.c
    │   └── 说明.md
    │
    └── main.c                   # 主循环装配（胶水层）
```

## 模块依赖关系

```
                    types.h (共享)
                       │
         ┌──────┬──────┼──────┬──────┬──────┐
         ▼      ▼      ▼      ▼      ▼      ▼
       player enemy  combat render  level   core
          │     │       │      │      │      │
          │     │       │      ├──────┤      │
          │     │       │      │ level │      │
          │     │       │      └──────┘      │
          └─────┴───────┴───────────────────┘
                          │
                          ▼
                       main.c
```

- 实线箭头 = `#include` 依赖
- player/enemy/combat/render/level/core 互相之间**不直接依赖**，只通过各自的 `*_api.h` 通信
- main.c 依赖所有 `*_api.h`，是唯一的聚合点

## 模块职责

| 模块 | 目录 | 职责 | 核心数据结构 |
|------|------|------|-------------|
| player | `src/player/` | 输入→移动/跳跃/射击、平台碰撞、受伤/死亡 | `PlayerData g_player`（单例） |
| enemy | `src/enemy/` | AI 状态机（IDLE→PATROL→CHASE→ATTACK）、敌人数组池管理 | `EnemyData g_enemies[20]`（静态数组池） |
| combat | `src/combat/` | 子弹管理、AABB 碰撞检测、战斗事件队列、连击计数 | `BulletData g_bullets[30]` + `Event g_events[32]` |
| render | `src/render/` | 纹理/音频资源加载、精灵绘制、背景/UI、震屏/hitstop 特效 | `Texture2D g_tex[TEX_COUNT]` |
| level | `src/level/` | 平台生成、摄像机跟随（lerp）、敌人生成队列、流程状态机 | `PlatformData g_platforms[50]` + `SpawnRequest g_spawnQueue[8]` |
| core | `src/core/` | raylib 封装（窗口/帧控制/输入）、数学工具函数 | 无内部状态 |
| main | `src/main.c` | 模块初始化和每帧调度 | 无内部状态 |

## 数据流向

### 逻辑帧（每帧执行顺序）

```
 1. Level_Update()          ← 摄像机跟随 + 敌人生成计时
 2. Level_GetPlatforms()    ← 取出平台列表
 3. Player_Update()         ← 输入→物理→碰撞，返回 PlayerData
 4. Bullet_Create()         ← 消费 pd.wantsShoot（玩家子弹）
 5. processSpawns()         ← 消费 level 产出的 SpawnRequest
 6. Enemy_UpdateAll()       ← AI 状态机 + 物理
 7. Bullet_Create()         ← 消费敌人 wantsShoot（敌人子弹）
 8. Bullet_UpdateAll()      ← 移动全部子弹
 9. Combat_Update()         ← 碰撞检测 → 写事件队列
10. processCombatEvents()   ← 消费事件：扣血/加分/死亡
```

### 渲染帧

```
Core_BeginFrame
  Render_GameBackground     (最远层，视差滚动)
  Render_Platform[]         (地形)
  Render_Enemy[]            (敌人)
  Render_Bullet[]           (子弹)
  Render_Player             (玩家)
  Render_UI                 (HUD，最顶层)
  Render_Pause              (暂停遮罩，覆盖一切)
Core_EndFrame
```

### 事件流

```
Combat_Update() 碰撞命中
       │
       ▼
  g_events[] 事件队列
       │
       ├──→ VFX_ProcessEvents()   震屏/hitstop（先消费）
       │
       └──→ processCombatEvents() 扣血/加分/死亡判定（后消费）
```

## 设计原则

1. **数据定义行为，代码定义规则**：所有游戏数值集中在 `types.h` 的常量区，行为由各模块代码驱动
2. **模块间零直接依赖**：每个模块只暴露 `*_api.h`，实现细节（`.c`）完全私有
3. **事件驱动解耦**：combat 产出 `Event`，render 和 main.c 分别消费，互不干扰
4. **胶水层在 main.c**：模块不互相调用，所有协调逻辑集中在主循环的10步顺序中
5. **意图标志位**：player/enemy 设 `wantsShoot`，main.c 消费并调 `Bullet_Create`，模块不跨层调 API

## 游戏机制

- **视角**：横版侧视，摄像机平滑跟随（lerp 系数 0.15）
- **操作**：方向键/WASD 移动，空格/W/↑ 跳跃，Z/J/鼠标左键 射击
- **死亡**：HP 归零 → 扣命 → 命归零 = 游戏结束；有命 = 满血重生
- **敌人 AI**：巡逻 → 发现玩家 → 追击 → 够近 → 站桩射击
- **碰撞**：AABB 矩形重叠检测，检测到即发射事件
