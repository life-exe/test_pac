#pragma once

#include "GameSnapshot.h"
#include "Types.h"

namespace LifeExe::Core
{

class Game
{
public:
    Game() = default;

    void update(float dt);

    GameState state() const { return m_state; }

    float totalTime() const { return m_totalTime; }

    GameSnapshot snapshot() const { return {.state = m_state, .totalTime = m_totalTime}; }

private:
    GameState m_state{GameState::Playing};
    float m_totalTime{0.0f};
};

}  // namespace LifeExe::Core