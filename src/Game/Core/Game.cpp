#include "Game.h"

namespace LifeExe::Core
{

void Game::update(float dt)
{
    if (m_state == GameState::Playing)
    {
        m_totalTime += dt;
    }
}

}  // namespace LifeExe::Core
