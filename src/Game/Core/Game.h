#pragma once

#include "Command.h"
#include "GameSnapshot.h"
#include "Maze.h"
#include "Player.h"
#include "Types.h"

namespace LifeExe::Core
{

class Game
{
public:
    /// Starts on the built-in classic level
    /// @throws std::bad_expected_access if the built-in level fails to parse
    Game();

    /// Starts on the given maze
    /// @param maze Parsed maze; the game takes ownership of it
    explicit Game(Maze maze);

    void update(float dt);
    void handleCommand(Command command);
    void reset();

    GameState state() const { return m_state; }

    double totalTime() const { return m_totalTime; }

    GameSnapshot snapshot() const
    {
        return {.state = m_state,
            .totalTime = m_totalTime,
            .mazeRows = m_maze.rows(),
            .mazeCols = m_maze.cols(),
            .tiles = m_maze.tiles(),
            .player = {.position = m_player.position(),
                .direction = m_player.direction(),
                .facing = m_player.facing(),
                .isMoving = m_player.isMoving()}};
    }

private:
    Maze m_maze;
    // Declared after m_maze: it is built from the maze's player spawn
    Player m_player;
    GameState m_state{GameState::Playing};
    double m_totalTime{0.0};
};

}  // namespace LifeExe::Core
