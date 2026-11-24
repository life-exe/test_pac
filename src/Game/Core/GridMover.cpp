#include "GridMover.h"

namespace LifeExe::Core
{

namespace
{

bool isInside(const Maze& maze, GridPos pos)
{
    return pos.row >= 0 && pos.row < maze.rows() && pos.col >= 0 && pos.col < maze.cols();
}

}  // namespace

bool canEnter(const Maze& maze, Mover mover, GridPos from, Direction direction)
{
    if (direction == Direction::None)
    {
        return false;
    }
    const GridPos target{from + directionDelta(direction)};
    if (maze.isWalkable(target, mover))
    {
        return true;
    }
    return maze.tileAt(from) == TileType::Tunnel && !isInside(maze, target) && target.row == from.row;
}

bool moversCollide(const GridMover& first, const GridMover& second)
{
    if (first.collisionTile() == second.collisionTile())
    {
        return true;
    }
    const Direction firstDir{first.direction()};
    const Direction secondDir{second.direction()};
    if (firstDir == Direction::None || firstDir != opposite(secondDir))
    {
        return false;
    }
    const GridPos firstNext{first.tile() + directionDelta(firstDir)};
    const GridPos secondNext{second.tile() + directionDelta(secondDir)};
    return firstNext == second.tile() && secondNext == first.tile() && (first.progress() + second.progress()) >= 1.0f;
}

GridMover::GridMover(GridPos tile, Direction facing) : m_tile{tile}, m_facing{facing} {}

void GridMover::update(const Maze& maze, const MoveStep& step, Direction& requested)
{
    float distance{step.speed * step.dt};
    while (stepCell(maze, step.mover, distance, requested))
    {
    }
}

bool GridMover::stepCell(const Maze& maze, Mover mover, float& distance, Direction& requested)
{
    if (m_progress == 0.0f)
    {
        decideAtCentre(maze, mover, requested);
        if (!m_isMoving)
        {
            distance = 0.0f;
            return false;
        }
    }
    else
    {
        tryReverse(maze, requested);
    }

    const float toNextCell{1.0f - m_progress};
    if (distance < toNextCell)
    {
        m_progress += distance;
        if (m_progress > 0.0f)
        {
            m_entryDirection = Direction::None;
        }
        distance = 0.0f;
        return false;
    }
    distance -= toNextCell;
    arriveAtNextCell(maze);
    return true;
}

Vec2f GridMover::position() const
{
    const GridPos delta{directionDelta(m_currentDirection)};
    return {static_cast<float>(m_tile.col) + 0.5f + (static_cast<float>(delta.col) * m_progress),
        static_cast<float>(m_tile.row) + 0.5f + (static_cast<float>(delta.row) * m_progress)};
}

void GridMover::arriveAtNextCell(const Maze& maze)
{
    const GridPos next{m_tile + directionDelta(m_currentDirection)};
    m_tile = isInside(maze, next) ? next : maze.tunnelExit(m_tile);
    m_entryDirection = m_currentDirection;
    m_progress = 0.0f;
}

void GridMover::decideAtCentre(const Maze& maze, Mover mover, Direction& requested)
{
    if (canEnter(maze, mover, m_tile, requested))
    {
        m_currentDirection = requested;
        requested = Direction::None;
    }
    if (canEnter(maze, mover, m_tile, m_currentDirection))
    {
        m_facing = m_currentDirection;
        m_isMoving = true;
        return;
    }
    m_currentDirection = Direction::None;
    m_entryDirection = Direction::None;
    m_isMoving = false;
    m_progress = 0.0f;
}

void GridMover::tryReverse(const Maze& maze, Direction& requested)
{
    if (requested == Direction::None || requested != opposite(m_currentDirection))
    {
        return;
    }
    // Beyond the map edge the reversal waits for the tunnel jump: that cell cannot become m_tile
    const GridPos next{m_tile + directionDelta(m_currentDirection)};
    if (!isInside(maze, next))
    {
        return;
    }
    m_tile = next;
    m_progress = 1.0f - m_progress;
    m_currentDirection = requested;
    m_facing = requested;
    requested = Direction::None;
}

bool GridMover::reverse(const Maze& maze, Mover mover)
{
    if (m_progress == 0.0f)
    {
        const Direction baseDir{m_entryDirection != Direction::None ? m_entryDirection : m_currentDirection};
        if (baseDir == Direction::None)
        {
            return false;
        }
        const Direction reversed{opposite(baseDir)};
        if (!canEnter(maze, mover, m_tile, reversed))
        {
            return false;
        }
        m_currentDirection = reversed;
        m_facing = reversed;
        m_entryDirection = reversed;
        return true;
    }
    if (m_currentDirection == Direction::None)
    {
        return false;
    }
    // Beyond the map edge the reversal cannot shift m_tile outside the grid
    const GridPos next{m_tile + directionDelta(m_currentDirection)};
    if (!isInside(maze, next))
    {
        return false;
    }
    const Direction reversed{opposite(m_currentDirection)};
    m_tile = next;
    m_progress = 1.0f - m_progress;
    m_currentDirection = reversed;
    m_facing = reversed;
    return true;
}

GridPos GridMover::collisionTile() const
{
    if (m_progress >= 0.5f && m_currentDirection != Direction::None)
    {
        return m_tile + directionDelta(m_currentDirection);
    }
    return m_tile;
}

}  // namespace LifeExe::Core
