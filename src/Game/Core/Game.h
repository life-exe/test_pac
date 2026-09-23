#pragma once

#include "Command.h"
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

    void setState(GameState state) { m_state = state; }

    double totalTime() const { return m_totalTime; }

    int score() const { return m_score; }

private:
    GameState m_state{GameState::Playing};
    double m_totalTime{0.0};
    int m_score{0};
};

}  // namespace LifeExe::Core