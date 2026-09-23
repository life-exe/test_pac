#include "Game.h"

namespace Game::Core
{

void Game::update(float dt)
{
    if (m_state == GameState::Playing)
    {
        m_totalTime += dt;
    }
}

}  // namespace Game::Core
