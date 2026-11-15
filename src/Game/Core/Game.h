#pragma once

#include "Command.h"
#include "GameSnapshot.h"
#include "Types.h"

namespace LifeExe::Core
{

class Game
{
public:
    Game() = default;

    void update(float dt);
    void handleCommand(Command command);
    void reset();

    GameState state() const { return m_state; }

    double totalTime() const { return m_totalTime; }

    GameSnapshot snapshot() const { return {.state = m_state, .totalTime = m_totalTime}; }

private:
    GameState m_state{GameState::Playing};
    double m_totalTime{0.0};
};

}  // namespace LifeExe::Core
