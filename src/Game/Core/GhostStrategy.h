#pragma once

#include <functional>

#include "Types.h"

namespace LifeExe::Core
{

struct GhostContext
{
    GridPos playerPos;
    Direction playerDir{Direction::None};
    GridPos rubyPos;
    GridPos ghostPos;
    GridPos scatterCorner;
};

using GhostStrategy = std::function<GridPos(const GhostContext&)>;

GridPos rubyStrategy(const GhostContext& context);
GridPos rosieStrategy(const GhostContext& context);
GridPos indigoStrategy(const GhostContext& context);
GridPos coralStrategy(const GhostContext& context);

}  // namespace LifeExe::Core
