#include "Player.h"

#include "GameConstants.h"

namespace LifeExe::Core
{

Player::Player(GridPos spawn) : m_mover{spawn, Direction::Left} {}

void Player::update(const Maze& maze, float dt)
{
    m_mover.update(maze, {.mover = Mover::Player, .speed = c_playerSpeed, .dt = dt}, m_queuedDirection);
    if (m_mover.isMoving())
    {
        m_animTime += dt;
    }
}

}  // namespace LifeExe::Core
