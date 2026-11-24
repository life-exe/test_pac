#include "GhostStrategy.h"

namespace LifeExe::Core
{

namespace
{

// Coral chases only while farther than 8 tiles; squared to stay in integers
constexpr int c_coralChaseRadiusSq{64};

int squaredDistance(GridPos lhs, GridPos rhs)
{
    const GridPos delta{lhs - rhs};
    return (delta.row * delta.row) + (delta.col * delta.col);
}

}  // namespace

GridPos rubyStrategy(const GhostContext& context)
{
    return context.playerPos;
}

GridPos rosieStrategy(const GhostContext& context)
{
    return context.playerPos + (4 * directionDelta(context.playerDir));
}

GridPos indigoStrategy(const GhostContext& context)
{
    const GridPos pivot{context.playerPos + (2 * directionDelta(context.playerDir))};
    return context.rubyPos + (2 * (pivot - context.rubyPos));
}

GridPos coralStrategy(const GhostContext& context)
{
    if (squaredDistance(context.ghostPos, context.playerPos) > c_coralChaseRadiusSq)
    {
        return context.playerPos;
    }
    return context.scatterCorner;
}

}  // namespace LifeExe::Core
