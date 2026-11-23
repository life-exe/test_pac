#pragma once

#include <array>
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

struct GhostSnapshot
{
    GhostId id{GhostId::Ruby};
    Vec2f position;
    Direction direction{Direction::None};
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
    std::array<GhostSnapshot, 4> ghosts;
    int score{0};
    int highScore{0};
    int remainingPellets{0};
};

}  // namespace LifeExe::Core
