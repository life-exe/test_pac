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

void Game::handleCommand(Command command)
{
    switch (command)
    {
        case Command::Pause:
            if (m_state == GameState::Playing)
            {
                m_state = GameState::Paused;
            }
            break;

        case Command::Resume:
            if (m_state == GameState::Paused)
            {
                m_state = GameState::Playing;
            }
            break;

        case Command::Reset: reset(); break;

        case Command::Quit: m_state = GameState::GameOver; break;

        case Command::None: break;
    }
}

void Game::reset()
{
    m_state = GameState::Playing;
    m_totalTime = 0.0;
    m_score = 0;
}

}  // namespace LifeExe::Core
