cmake_minimum_required(VERSION 3.16)
project(EnchantedWarrior LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==================== 依赖：raylib (vendored) ====================
# raylib 源码在 vendor/raylib/，不用联网下载
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/raylib ${CMAKE_BINARY_DIR}/raylib_build)

# ==================== 6个独立模块 ====================

# 共享类型（所有人依赖）
set(TYPES_HDR src/types/types.h)

# 队友1: 玩家 — player/
set(PLAYER_SRC src/player/player.c)
set(PLAYER_API src/player/player_api.h ${TYPES_HDR})

# 队友2: 敌人AI — enemy/
set(ENEMY_SRC  src/enemy/enemy.c)
set(ENEMY_API  src/enemy/enemy_api.h ${TYPES_HDR})

# 队友3: 战斗判定 — combat/
set(COMBAT_SRC src/combat/combat.c)
set(COMBAT_API src/combat/combat_api.h ${TYPES_HDR})

# 队友4: 渲染/表现 — render/
set(RENDER_SRC src/render/render.c)
set(RENDER_API src/render/render_api.h ${TYPES_HDR})

# 队友5: 关卡/流程 — level/
set(LEVEL_SRC  src/level/level.c)
set(LEVEL_API  src/level/level_api.h ${TYPES_HDR})

# 队友6: 核心/主循环 — core/
set(CORE_SRC   src/core/core.c src/main.c)
set(CORE_API   src/core/core_api.h ${TYPES_HDR})

# 所有源文件
set(ALL_SOURCES
    ${PLAYER_SRC} ${ENEMY_SRC} ${COMBAT_SRC}
    ${RENDER_SRC} ${LEVEL_SRC}  ${CORE_SRC}
)

# ==================== 可执行文件 ====================
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor/raylib/src
    ${CMAKE_SOURCE_DIR}/vendor/raylib/src/external/glfw/include
)

# 链接 raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib m pthread dl)

# ==================== 复制资源到构建目录 ====================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
    COMMENT "Copying resources to build directory..."
)

# ==================== 编译选项 ====================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
