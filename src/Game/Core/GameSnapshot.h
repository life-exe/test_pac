#pragma once

#include "Types.h"

namespace LifeExe::Core
{

// Plain copy of the core state: the view reads it and cannot change the core
struct GameSnapshot
{
    GameState state{GameState::Playing};
    double totalTime{0.0};
};

}  // namespace LifeExe::Core