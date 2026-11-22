#pragma once

#include "GridMover.h"
#include "Types.h"

namespace LifeExe::Core
{

class Player
{
public:
    explicit Player(GridPos spawn);

    // Remembers the wanted direction; it is applied at the next cell centre where the turn is possible
    void queueDirection(Direction direction) { m_queuedDirection = direction; }

    void update(const Maze& maze, float dt);

    Vec2f position() const { return m_mover.position(); }

    Direction direction() const { return m_mover.direction(); }

    Direction facing() const { return m_mover.facing(); }

    bool isMoving() const { return m_mover.isMoving(); }

private:
    GridMover m_mover;
    Direction m_queuedDirection{Direction::None};
};

}  // namespace LifeExe::Core
