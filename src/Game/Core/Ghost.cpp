#include "Ghost.h"

#include <array>
#include <cmath>
#include <limits>

#include "GameConstants.h"

namespace LifeExe::Core
{

namespace
{

constexpr std::array<Direction, 4> c_directionPriority{Direction::Up, Direction::Left, Direction::Down, Direction::Right};

int squaredDistance(GridPos lhs, GridPos rhs)
{
    const GridPos delta{lhs - rhs};
    return (delta.row * delta.row) + (delta.col * delta.col);
}

}  // namespace

Ghost::Ghost(const GhostConfig& config)
    : m_id{config.id}, m_mover{config.spawn, Direction::Left}, m_scatterCorner{config.scatterCorner}, m_inHouse{config.inHouse},
      m_releaseDelay{config.releaseDelay}
{
}

void Ghost::update(const Maze& maze, float dt)
{
    updateToward(maze, dt, m_scatterCorner);
}

void Ghost::updateToward(const Maze& maze, float dt, GridPos targetTile)
{
    if (!std::isfinite(dt) || dt < 0.0f)
    {
        return;
    }

    if (m_releaseDelay > 0.0f)
    {
        if (dt < m_releaseDelay)
        {
            m_releaseDelay -= dt;
            return;
        }
        dt -= m_releaseDelay;
        m_releaseDelay = 0.0f;
    }

    float distance{c_ghostSpeed * dt};
    for (;;)
    {
        if (m_inHouse && m_mover.progress() == 0.0f && m_mover.tile() == maze.houseExitTile())
        {
            m_inHouse = false;
        }

        const GridPos effectiveTarget{m_inHouse ? maze.houseExitTile() : targetTile};
        Direction requested{Direction::None};
        if (m_mover.progress() == 0.0f)
        {
            requested = chooseNextDirection(maze, effectiveTarget);
        }

        if (!m_mover.stepCell(maze, currentMover(), distance, requested))
        {
            return;
        }
    }
}

Direction Ghost::chooseNextDirection(const Maze& maze, GridPos targetTile) const
{
    const Mover mover{currentMover()};
    const Direction reverseDir{m_inHouse ? Direction::None : opposite(m_mover.direction())};

    Direction bestDir{Direction::None};
    int bestDistSq{std::numeric_limits<int>::max()};

    for (const Direction dir : c_directionPriority)
    {
        if (dir == reverseDir || !canEnter(maze, mover, m_mover.tile(), dir))
        {
            continue;
        }
        const GridPos neighbour{m_mover.tile() + directionDelta(dir)};
        const int distSq{squaredDistance(neighbour, targetTile)};
        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestDir = dir;
        }
    }

    if (bestDir != Direction::None)
    {
        return bestDir;
    }

    // Dead-end fallback: reversing is allowed only when no other path exists
    if (reverseDir != Direction::None && canEnter(maze, mover, m_mover.tile(), reverseDir))
    {
        return reverseDir;
    }

    return Direction::None;
}

}  // namespace LifeExe::Core
