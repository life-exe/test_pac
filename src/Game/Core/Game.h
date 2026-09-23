#pragma once

#include "Types.h"

namespace Game::Core
{

class Game
{
public:
    Game() = default;

    void update(float dt);

    GameState state() const { return m_state; }

    void setState(GameState state) { m_state = state; }

    float totalTime() const { return m_totalTime; }

    int score() const { return m_score; }

private:
    GameState m_state{GameState::Playing};
    float m_totalTime{0.0F};
    int m_score{0};
};

}  // namespace Game::Core