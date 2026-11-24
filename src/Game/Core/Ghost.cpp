#include "Ghost.h"

#include <array>
#include <cmath>
#include <limits>
#include <utility>

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

float speedFor(GhostMode mode)
{
    switch (mode)
    {
        case GhostMode::Frightened: return c_ghostSpeed * c_frightenedSpeedMultiplier;
        case GhostMode::Eaten: return c_ghostSpeed * c_eatenSpeedMultiplier;
        case GhostMode::Scatter:
        case GhostMode::Chase: return c_ghostSpeed;
    }
    return c_ghostSpeed;
}

// Ghost.h is a fixed contract without private helpers, so every update overload shares this one movement loop
// through references to the ghost's state; the ghost itself is read only through its public queries
struct GhostMotion
{
    GridMover& mover;
    GhostMode& mode;
    bool& inHouse;
    bool& enteringHouse;
    bool& holdReversedDirection;
    float& releaseDelay;
    GridPos homeTile;
};

struct MotionInputs
{
    const Ghost& ghost;
    GridPos waveTarget;
    IRandomSource* random{nullptr};  // null on the legacy paths: Frightened then steers to the target
    GhostMode waveMode{GhostMode::Scatter};
};

Mover moverFor(const GhostMotion& motion)
{
    return (motion.inHouse || motion.mode == GhostMode::Eaten) ? Mover::GhostUsingDoor : Mover::Ghost;
}

// Eaten eyes aim at the cell above the door first: a target behind the house wall would trap the greedy
// no-reverse choice in a loop around the house, so the descent through the door is a separate phase
void updateHouseTransitions(const Maze& maze, const GhostMotion& motion, GhostMode waveMode)
{
    const GridPos tile{motion.mover.tile()};
    if (motion.inHouse && tile == maze.houseExitTile())
    {
        motion.inHouse = false;
    }
    if (motion.mode != GhostMode::Eaten)
    {
        return;
    }
    if (motion.enteringHouse && tile == motion.homeTile)
    {
        // The revived ghost leaves the house by the regular exit route, with no extra wait
        motion.enteringHouse = false;
        motion.inHouse = true;
        motion.releaseDelay = 0.0f;
        motion.mode = waveMode;
    }
    else if (tile == maze.houseExitTile())
    {
        motion.enteringHouse = true;
    }
}

Direction decideAtCentre(const Maze& maze, const GhostMotion& motion, const MotionInputs& inputs)
{
    if (!motion.inHouse && motion.mode == GhostMode::Frightened && inputs.random != nullptr)
    {
        return inputs.ghost.chooseRandomDirection(maze, *inputs.random);
    }
    GridPos target{inputs.waveTarget};
    if (motion.inHouse)
    {
        target = maze.houseExitTile();
    }
    else if (motion.mode == GhostMode::Eaten)
    {
        target = motion.enteringHouse ? motion.homeTile : maze.houseExitTile();
    }
    return inputs.ghost.chooseNextDirection(maze, target);
}

void advanceMovement(const Maze& maze, float dt, const GhostMotion& motion, const MotionInputs& inputs)
{
    if (!std::isfinite(dt) || dt < 0.0f)
    {
        return;
    }

    if (motion.releaseDelay > 0.0f)
    {
        if (dt < motion.releaseDelay)
        {
            motion.releaseDelay -= dt;
            return;
        }
        dt -= motion.releaseDelay;
        motion.releaseDelay = 0.0f;
    }

    float distance{speedFor(motion.mode) * dt};
    for (;;)
    {
        Direction requested{Direction::None};
        if (motion.mover.progress() == 0.0f)
        {
            const GhostMode modeBefore{motion.mode};
            updateHouseTransitions(maze, motion, inputs.waveMode);
            if (motion.mode != modeBefore)
            {
                // The rest of this step was measured at the Eaten speed; the revived ghost covers it at its own
                distance *= speedFor(motion.mode) / speedFor(modeBefore);
            }

            if (motion.holdReversedDirection)
            {
                if (distance > 0.0f)
                {
                    motion.holdReversedDirection = false;
                }
            }
            else
            {
                requested = decideAtCentre(maze, motion, inputs);
            }
        }

        if (!motion.mover.stepCell(maze, moverFor(motion), distance, requested))
        {
            return;
        }
    }
}

}  // namespace

Ghost::Ghost(const GhostConfig& config) : Ghost{config, nullptr} {}

Ghost::Ghost(const GhostConfig& config, GhostStrategy strategy)
    : m_id{config.id}, m_mover{config.spawn, Direction::Left}, m_scatterCorner{config.scatterCorner},
      m_homeTile{config.homeTile == GridPos{} ? config.spawn : config.homeTile}, m_strategy{std::move(strategy)}, m_inHouse{config.inHouse},
      m_releaseDelay{config.releaseDelay}
{
}

void Ghost::update(const Maze& maze, float dt)
{
    updateToward(maze, dt, m_scatterCorner);
}

void Ghost::update(const Maze& maze, float dt, const GhostContext& context)
{
    updateToward(maze, dt, resolveTarget(context));
}

// Without a random source or a wave mode, Frightened steers to the target and a revived ghost starts in Scatter
void Ghost::updateToward(const Maze& maze, float dt, GridPos targetTile)
{
    advanceMovement(maze, dt, {m_mover, m_mode, m_inHouse, m_enteringHouse, m_holdReversedDirection, m_releaseDelay, m_homeTile},
        {.ghost = *this, .waveTarget = targetTile, .random = nullptr, .waveMode = GhostMode::Scatter});
}

void Ghost::update(const Maze& maze, float dt, const GhostUpdateParams& params)
{
    advanceMovement(maze, dt, {m_mover, m_mode, m_inHouse, m_enteringHouse, m_holdReversedDirection, m_releaseDelay, m_homeTile},
        {.ghost = *this, .waveTarget = resolveTarget(params.context), .random = &params.random, .waveMode = params.waveMode});
}

Direction Ghost::chooseRandomDirection(const Maze& maze, IRandomSource& random) const
{
    const Mover mover{currentMover()};
    const Direction reverseDir{m_inHouse ? Direction::None : opposite(m_mover.direction())};

    std::array<Direction, 4> candidates{};
    std::size_t count{0};
    for (const Direction dir : c_directionPriority)
    {
        if (dir != reverseDir && canEnter(maze, mover, m_mover.tile(), dir))
        {
            candidates[count] = dir;
            ++count;
        }
    }

    if (count > 0)
    {
        return candidates[random.nextIndex(count) % count];
    }

    // Dead-end fallback when no forward or side turn is open
    if (reverseDir != Direction::None && canEnter(maze, mover, m_mover.tile(), reverseDir))
    {
        return reverseDir;
    }

    return Direction::None;
}

void Ghost::setMode(GhostMode mode, const Maze& maze, bool allowReverse)
{
    if (m_mode == mode)
    {
        return;
    }
    m_mode = mode;
    // Eaten has no use for a held reversal, and any other mode ends the descent into the house
    if (mode == GhostMode::Eaten)
    {
        m_holdReversedDirection = false;
    }
    else
    {
        m_enteringHouse = false;
    }
    if (!m_inHouse && allowReverse)
    {
        const bool atCentre{m_mover.progress() == 0.0f};
        if (m_mover.reverse(maze, currentMover()) && atCentre)
        {
            m_holdReversedDirection = true;
        }
    }
}

GridPos Ghost::resolveTarget(const GhostContext& context) const
{
    if (m_mode == GhostMode::Chase && m_strategy)
    {
        return m_strategy(context);
    }
    return m_scatterCorner;
}

Direction Ghost::chooseNextDirection(const Maze& maze, GridPos targetTile) const
{
    const Mover mover{currentMover()};
    // Inside the house and on the way down to it the ghost may turn back, otherwise it could not pass the door
    const bool allowAnyTurn{m_inHouse || m_enteringHouse};
    const Direction reverseDir{allowAnyTurn ? Direction::None : opposite(m_mover.direction())};

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
