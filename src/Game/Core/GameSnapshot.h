#pragma once

#include <span>

#include "Types.h"

namespace LifeExe::Core
{

/// Plain copy of the core state: the view reads it and cannot change the core.
/// The tiles span points into the maze owned by Game and stays valid only until
/// the next call to update, handleCommand or reset.
struct GameSnapshot
{
    GameState state{GameState::Playing};
    double totalTime{0.0};
    int mazeRows{0};                  ///< Number of maze rows
    int mazeCols{0};                  ///< Number of maze columns
    std::span<const TileType> tiles;  ///< Row-major cells: index is row * mazeCols + col
};

}  // namespace LifeExe::Core
