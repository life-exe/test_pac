#pragma once

#include "GhostStrategy.h"
#include "GridMover.h"
#include "Maze.h"
#include "Types.h"
#include "RandomSource.h"

namespace LifeExe::Core
{

struct GhostConfig
{
    GhostId id{GhostId::Ruby};
    GridPos spawn;
    GridPos scatterCorner;
    bool inHouse{false};
    float releaseDelay{0.0f};
    GridPos homeTile{};
};

struct GhostUpdateParams
{
    const GhostContext& context;
    IRandomSource& random;
    GhostMode waveMode{GhostMode::Scatter};
};

class Ghost
{
public:
    explicit Ghost(const GhostConfig& config);
    Ghost(const GhostConfig& config, GhostStrategy strategy);

    void update(const Maze& maze, float dt);
    void update(const Maze& maze, float dt, const GhostContext& context);
    void updateToward(const Maze& maze, float dt, GridPos targetTile);
    void update(const Maze& maze, float dt, const GhostUpdateParams& params);

    Direction chooseNextDirection(const Maze& maze, GridPos targetTile) const;
    void setMode(GhostMode mode, const Maze& maze, bool allowReverse = true);

    GhostId id() const { return m_id; }
    GridPos tile() const { return m_mover.tile(); }
    Vec2f position() const { return m_mover.position(); }
    Direction direction() const { return m_mover.direction(); }
    GridPos scatterCorner() const { return m_scatterCorner; }
    GhostMode mode() const { return m_mode; }
    bool isInHouse() const { return m_inHouse; }
    float releaseDelay() const { return m_releaseDelay; }
    const GridMover& mover() const { return m_mover; }
    Direction chooseRandomDirection(const Maze& maze, IRandomSource& random) const;

private:
    Mover currentMover() const { return (m_inHouse || m_mode == GhostMode::Eaten) ? Mover::GhostUsingDoor : Mover::Ghost; }
    GridPos resolveTarget(const GhostContext& context) const;

    GhostId m_id;
    GridMover m_mover;
    GridPos m_scatterCorner;
    GridPos m_homeTile;
    GhostStrategy m_strategy;
    GhostMode m_mode{GhostMode::Scatter};
    bool m_inHouse{false};
    bool m_holdReversedDirection{false};
    float m_releaseDelay{0.0f};
    bool m_enteringHouse{false};
};

}  // namespace LifeExe::Core
