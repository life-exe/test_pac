#pragma once

#include "Maze.h"
#include "Types.h"

namespace LifeExe::Core
{

// Can a mover standing in the centre of 'from' step into the next cell in 'direction'?
// The only exception to "a cell outside the map is a wall" is the tunnel mouth at the map edge.
bool canEnter(const Maze& maze, Mover mover, GridPos from, Direction direction);

// Parameters of a single movement step along the maze
struct MoveStep
{
    Mover mover{Mover::Player};
    float speed{0.0f};
    float dt{0.0f};
};

// Movement model "tile + progress", shared by the player and the ghosts
class GridMover
{
public:
    GridMover(GridPos tile, Direction facing);

    // Moves along the maze by step.speed * step.dt tiles. 'requested' is the wanted direction: it is
    // consumed at a cell centre when the turn is possible, and an opposite request reverses at once.
    void update(const Maze& maze, const MoveStep& step, Direction& requested);

    GridPos tile() const { return m_tile; }

    Direction direction() const { return m_currentDirection; }

    Direction facing() const { return m_facing; }

    bool isMoving() const { return m_isMoving; }

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
