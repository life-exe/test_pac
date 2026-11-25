#pragma once

#include <array>
#include <memory>
#include <vector>

#include "Command.h"
#include "GameSnapshot.h"
#include "Ghost.h"
#include "Maze.h"
#include "Player.h"
#include "Types.h"
#include "GameConstants.h"
#include "GameEvents.h"

namespace LifeExe::Core
{

class Game
{
public:
    Game();
    explicit Game(Maze maze);
    Game(Maze maze, std::unique_ptr<IRandomSource> random);

    void update(float dt);
    void handleCommand(Command command);
    void reset();  // keeps m_highScore intact
    void setHighScore(int highScore);

    GameState state() const { return m_state; }
    double totalTime() const { return m_totalTime; }
    GameSnapshot snapshot() const;
    std::vector<GameEvent> takeEvents();

private:
    static std::array<Ghost, 4> createGhosts(const Maze& maze);
    void updateSubstep(float stepDt);
    void consumePelletAtPlayer();
    std::array<GhostSnapshot, 4> ghostSnapshots() const;
    void updateWaveTimer(float stepDt);
    void advancePhaseTimer(float stepDt);
    void checkGhostCollisions();
    void respawnRound();
    void updateFrightenedTimer(float stepDt);
    void activateFrightened();

    Maze m_initialMaze;
    Maze m_maze;
    Player m_player;
    // Declared after m_maze: the ghosts are built from its spawns and corners
    std::array<Ghost, 4> m_ghosts;  // indexed by GhostId
    GameState m_state{GameState::MainMenu};
    double m_totalTime{0.0};
    int m_score{0};
    int m_highScore{0};

    GhostMode m_waveMode{GhostMode::Scatter};
    float m_waveTimer{c_scatterDuration};

    RoundPhase m_roundPhase{RoundPhase::Ready};
    float m_phaseTimer{c_readyDuration};
    int m_lives{c_startingLives};

    std::unique_ptr<IRandomSource> m_random;
    float m_frightenedTimer{0.0f};
    int m_eatenGhostCombo{0};

    std::vector<GameEvent> m_events;
};

}  // namespace LifeExe::Core
