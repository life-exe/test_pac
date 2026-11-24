#pragma once

#include "GridMover.h"
#include "Types.h"

namespace LifeExe::Core
{

class Player
{
public:
    explicit Player(GridPos spawn);

    void queueDirection(Direction direction) { m_queuedDirection = direction; }
    void update(const Maze& maze, float dt);

    Vec2f position() const { return m_mover.position(); }
    Direction direction() const { return m_mover.direction(); }
    Direction facing() const { return m_mover.facing(); }  // preserved when stopped before a wall
    bool isMoving() const { return m_mover.isMoving(); }
    float animTime() const { return m_animTime; }  // seconds, freezes when stopped
    GridPos tile() const;
    const GridMover& mover() const { return m_mover; }

private:
    GridMover m_mover;
    Direction m_queuedDirection{Direction::None};
    float m_animTime{0.0f};
};

}  // namespace LifeExe::Core
