#pragma once

#include <array>

#include "Command.h"
#include "GameSnapshot.h"
#include "Ghost.h"
#include "Maze.h"
#include "Player.h"
#include "Types.h"

namespace LifeExe::Core
{

class Game
{
public:
    Game();
    explicit Game(Maze maze);

    void update(float dt);
    void handleCommand(Command command);
    void reset();  // keeps m_highScore intact
    void setHighScore(int highScore);

    GameState state() const { return m_state; }
    double totalTime() const { return m_totalTime; }
    GameSnapshot snapshot() const;

private:
    void updateSubstep(float stepDt);
    void consumePelletAtPlayer();
    std::array<GhostSnapshot, 4> ghostSnapshots() const;

    Maze m_initialMaze;
    Maze m_maze;
    Player m_player;
    // Declared after m_maze: the ghosts are built from its spawns and corners
    std::array<Ghost, 4> m_ghosts;  // indexed by GhostId
    GameState m_state{GameState::Playing};
    double m_totalTime{0.0};
    int m_score{0};
    int m_highScore{0};
};

}  // namespace LifeExe::Core
