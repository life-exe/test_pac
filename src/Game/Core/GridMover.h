#pragma once

#include "Maze.h"
#include "Types.h"

namespace LifeExe::Core
{

// The only exception to "out-of-bounds is a wall" is a tunnel exit at the map edge.
bool canEnter(const Maze& maze, Mover mover, GridPos from, Direction direction);

struct MoveStep
{
    Mover mover{Mover::Player};
    float speed{0.0f};  // tiles per second
    float dt{0.0f};     // seconds
};

class GridMover
{
public:
    GridMover(GridPos tile, Direction facing);

    // requested is cleared to Direction::None once consumed.
    void update(const Maze& maze, const MoveStep& step, Direction& requested);

    // Advances at most to the next cell centre; returns true if a new cell centre was reached.
    bool stepCell(const Maze& maze, Mover mover, float& distance, Direction& requested);

    GridPos tile() const { return m_tile; }
    Direction direction() const { return m_currentDirection; }
    Direction facing() const { return m_facing; }  // preserved when stopped before a wall
    bool isMoving() const { return m_isMoving; }
    float progress() const { return m_progress; }
    Vec2f position() const;

private:
    void decideAtCentre(const Maze& maze, Mover mover, Direction& requested);
    void tryReverse(const Maze& maze, Direction& requested);
    void arriveAtNextCell(const Maze& maze);

    GridPos m_tile;
    Direction m_currentDirection{Direction::None};
    Direction m_facing;
    bool m_isMoving{false};
    float m_progress{0.0f};
};

}  // namespace LifeExe::Core
