#pragma once

#include <span>

#include "Types.h"

namespace LifeExe::Core
{

struct PlayerSnapshot
{
    Vec2f position;
    Direction direction{Direction::None};
    Direction facing{Direction::Left};
    bool isMoving{false};
    float animTime{0.0f};
};

// Read-only frame snapshot; tiles views core memory until the next update, handleCommand or reset.
struct GameSnapshot
{
    GameState state{GameState::Playing};
    double totalTime{0.0};
    int mazeRows{0};
    int mazeCols{0};
    std::span<const TileType> tiles;
    PlayerSnapshot player;
};

}  // namespace LifeExe::Core
