#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

#include "Game.h"
#include "GameConstants.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;

namespace
{

// A dead-end corridor with three pellets to the right of the spawn: two regular ones and a power pellet.
// Ghost spawns and the door exist only because the parser requires them.
constexpr std::string_view c_corridorLevel{"######\n"
                                           "#1234#\n"
                                           "##=###\n"
                                           "#P..o#\n"
                                           "######\n"};

constexpr Core::GridPos c_firstPellet{3, 2};
constexpr Core::GridPos c_secondPellet{3, 3};
constexpr Core::GridPos c_powerPellet{3, 4};
constexpr int c_corridorPellets{3};
constexpr int c_corridorFullScore{(2 * Core::c_pelletScore) + Core::c_powerPelletScore};

constexpr float secondsFor(float tiles)
{
    return tiles / Core::c_playerSpeed;
}

Core::Game makeCorridorGame()
{
    Core::Game game{Tests::makePlayingGame(c_corridorLevel)};
    game.handleCommand(Core::Command::MoveRight);
    return game;
}

Core::TileType tileAt(const Core::GameSnapshot& snapshot, Core::GridPos pos)
{
    const std::size_t index{
        (static_cast<std::size_t>(pos.row) * static_cast<std::size_t>(snapshot.mazeCols)) + static_cast<std::size_t>(pos.col)};
    return snapshot.tiles[index];
}

// Every ghost receives the wave mode, including those still in the house
void expectAllGhostsInMode(const Core::Game& game, Core::GhostMode mode)
{
    for (const Core::GhostSnapshot& ghost : game.snapshot().ghosts)
    {
        EXPECT_EQ(ghost.mode, mode) << "ghost " << static_cast<int>(ghost.id);
    }
}

// Margin around a wave boundary, so the checks never land exactly on the switch
constexpr float c_waveMargin{0.25f};

// The player is walled off from the ghost house, so long wave checks never end in a collision
constexpr std::string_view c_walledOffLevel{"###########\n"
                                            "#....1....#\n"
                                            "#.###=###.#\n"
                                            "#.# 234 #.#\n"
                                            "###########\n"
                                            "T....P....T\n"
                                            "###########\n"};

// Ruby starts in a dead end whose only exit leads to the player, so moving right always ends in a collision.
// The pellet behind the player is never reached while moving right, so the map cannot be cleared before the
// collision, whatever the exact eating moment is.
constexpr std::string_view c_collisionLevel{"#######\n"
                                            "#.P..1#\n"
                                            "###=###\n"
                                            "# 234 #\n"
                                            "#######\n"};

constexpr Core::GridPos c_collisionPelletBehind{1, 1};
constexpr Core::GridPos c_collisionPelletAhead{1, 3};

// Upper bound for phase loops, so a broken phase fails the test instead of hanging it
constexpr int c_maxPhaseSteps{640};  // 10 seconds of c_maxSubstep

// Wave and release timers tick only in RoundPhase::Active, so the Ready pause after Reset is skipped first
void skipReadyPhase(Core::Game& game)
{
    while (game.state() == Core::GameState::Playing && game.snapshot().roundPhase == Core::RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
}

// Advances in substeps while the game is Playing in the given phase; false if the phase never ends
bool runWhilePhase(Core::Game& game, Core::RoundPhase phase)
{
    for (int step{0}; step < c_maxPhaseSteps; ++step)
    {
        if (game.state() != Core::GameState::Playing || game.snapshot().roundPhase != phase)
        {
            return true;
        }
        game.update(Core::c_maxSubstep);
    }
    return false;
}

// Plays a round on c_collisionLevel until the player meets Ruby; stops on the substep of the collision
bool runIntoGhost(Core::Game& game)
{
    skipReadyPhase(game);
    game.handleCommand(Core::Command::MoveRight);
    return runWhilePhase(game, Core::RoundPhase::Active);
}

// A full death on c_collisionLevel: the collision and then the whole PlayerDying pause
bool loseLife(Core::Game& game)
{
    return runIntoGhost(game) && runWhilePhase(game, Core::RoundPhase::PlayerDying);
}

Core::Vec2f centreOf(Core::GridPos tile)
{
    return {static_cast<float>(tile.col) + 0.5f, static_cast<float>(tile.row) + 0.5f};
}

void expectSamePosition(Core::Vec2f actual, Core::Vec2f expected)
{
    EXPECT_FLOAT_EQ(actual.x, expected.x);
    EXPECT_FLOAT_EQ(actual.y, expected.y);
}

// Nothing in the round moves: the player and every ghost keep their position and direction
void expectEntitiesFrozen(const Core::GameSnapshot& before, const Core::GameSnapshot& after)
{
    expectSamePosition(after.player.position, before.player.position);
    EXPECT_EQ(after.player.direction, before.player.direction);
    for (std::size_t index{0}; index < before.ghosts.size(); ++index)
    {
        SCOPED_TRACE(testing::Message() << "ghost " << index);
        expectSamePosition(after.ghosts[index].position, before.ghosts[index].position);
        EXPECT_EQ(after.ghosts[index].direction, before.ghosts[index].direction);
    }
}

void expectAtSpawns(const Core::GameSnapshot& snapshot, const Core::Maze& maze)
{
    expectSamePosition(snapshot.player.position, centreOf(maze.playerSpawn()));
    for (const Core::GhostSnapshot& ghost : snapshot.ghosts)
    {
        SCOPED_TRACE(testing::Message() << "ghost " << static_cast<int>(ghost.id));
        expectSamePosition(ghost.position, centreOf(maze.ghostSpawn(ghost.id)));
    }
}

}  // namespace

TEST(GameCoreTest, InitialState)
{
    const Core::Game game;
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);
}

TEST(GameCoreTest, AccumulatesTotalTimeWhilePlaying)
{
    Core::Game game;
    game.update(0.5f);
    game.update(0.25f);

    EXPECT_DOUBLE_EQ(game.totalTime(), 0.75);
}

TEST(GameCoreTest, SnapshotCopiesCoreState)
{
    Core::Game game;
    game.update(0.5f);

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(snapshot.totalTime, 0.5);
}

TEST(GameCoreTest, GameStateToString)
{
    EXPECT_EQ(Core::toString(Core::GameState::Playing), "Playing");
    EXPECT_EQ(Core::toString(Core::GameState::Paused), "Paused");
    EXPECT_EQ(Core::toString(Core::GameState::GameOver), "GameOver");
    EXPECT_EQ(Core::toString(Core::GameState::Victory), "Victory");
}

TEST(GameCoreTest, HandlesPauseAndResumeCommands)
{
    Core::Game game;
    game.handleCommand(Core::Command::Pause);
    EXPECT_EQ(game.state(), Core::GameState::Paused);

    game.update(1.0f);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);

    game.handleCommand(Core::Command::Resume);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    game.update(0.25f);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.25);
}

TEST(GameCoreTest, HandlesTogglePauseCommand)
{
    Core::Game game;
    game.handleCommand(Core::Command::TogglePause);
    EXPECT_EQ(game.state(), Core::GameState::Paused);

    game.handleCommand(Core::Command::TogglePause);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    // A finished game cannot be paused or resumed
    game.handleCommand(Core::Command::Quit);
    game.handleCommand(Core::Command::TogglePause);
    EXPECT_EQ(game.state(), Core::GameState::GameOver);
}

TEST(GameCoreTest, HandlesResetCommand)
{
    Core::Game game;
    game.update(1.5f);
    game.handleCommand(Core::Command::Pause);

    game.handleCommand(Core::Command::Reset);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);
}

TEST(GameCoreTest, HandlesQuitCommand)
{
    Core::Game game;
    game.handleCommand(Core::Command::Quit);
    EXPECT_EQ(game.state(), Core::GameState::GameOver);
}

TEST(GameCoreTest, CommandToString)
{
    EXPECT_EQ(Core::toString(Core::Command::None), "None");
    EXPECT_EQ(Core::toString(Core::Command::Pause), "Pause");
    EXPECT_EQ(Core::toString(Core::Command::Resume), "Resume");
    EXPECT_EQ(Core::toString(Core::Command::TogglePause), "TogglePause");
    EXPECT_EQ(Core::toString(Core::Command::Reset), "Reset");
    EXPECT_EQ(Core::toString(Core::Command::Quit), "Quit");
    EXPECT_EQ(Core::toString(Core::Command::MoveUp), "MoveUp");
    EXPECT_EQ(Core::toString(Core::Command::MoveDown), "MoveDown");
    EXPECT_EQ(Core::toString(Core::Command::MoveLeft), "MoveLeft");
    EXPECT_EQ(Core::toString(Core::Command::MoveRight), "MoveRight");
}

// --- Pellets, score and victory ---

TEST(GameScoreTest, EatingPelletAddsPelletScore)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(1.0f));

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.score, Core::c_pelletScore);
    EXPECT_EQ(snapshot.highScore, Core::c_pelletScore);
    EXPECT_EQ(snapshot.remainingPellets, c_corridorPellets - 1);
    EXPECT_EQ(tileAt(snapshot, c_firstPellet), Core::TileType::Empty);
}

TEST(GameScoreTest, EatingPowerPelletAddsPowerPelletScore)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(2.0f));
    ASSERT_EQ(game.snapshot().score, 2 * Core::c_pelletScore);

    game.update(secondsFor(1.0f));
    EXPECT_EQ(game.snapshot().score, c_corridorFullScore);
    EXPECT_EQ(tileAt(game.snapshot(), c_powerPellet), Core::TileType::Empty);
}

TEST(GameScoreTest, HighScoreFollowsScore)
{
    Core::Game game{makeCorridorGame()};
    for (int step{0}; step < c_corridorPellets; ++step)
    {
        game.update(secondsFor(1.0f));
        const Core::GameSnapshot snapshot{game.snapshot()};
        EXPECT_EQ(snapshot.highScore, snapshot.score) << "step " << step;
    }
}

TEST(GameScoreTest, SetHighScoreKeepsLargerRecord)
{
    Core::Game game{makeCorridorGame()};
    game.setHighScore(500);
    EXPECT_EQ(game.snapshot().highScore, 500);

    game.setHighScore(100);
    EXPECT_EQ(game.snapshot().highScore, 500);
}

TEST(GameScoreTest, SetHighScoreBelowScoreIsOvertaken)
{
    Core::Game game{makeCorridorGame()};
    game.setHighScore(15);

    game.update(secondsFor(1.0f));
    EXPECT_EQ(game.snapshot().score, 10);
    EXPECT_EQ(game.snapshot().highScore, 15);

    game.update(secondsFor(1.0f));
    EXPECT_EQ(game.snapshot().score, 20);
    EXPECT_EQ(game.snapshot().highScore, 20);
}

TEST(GameScoreTest, ResetRestoresPelletsAndKeepsHighScore)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(2.0f));
    ASSERT_EQ(game.snapshot().score, 2 * Core::c_pelletScore);

    game.handleCommand(Core::Command::Reset);

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.score, 0);
    EXPECT_EQ(snapshot.highScore, 2 * Core::c_pelletScore);
    EXPECT_EQ(snapshot.remainingPellets, c_corridorPellets);
    EXPECT_EQ(tileAt(snapshot, c_firstPellet), Core::TileType::Pellet);
    EXPECT_EQ(tileAt(snapshot, c_secondPellet), Core::TileType::Pellet);
}

TEST(GameScoreTest, NoPelletsEatenWhilePaused)
{
    Core::Game game{makeCorridorGame()};
    game.handleCommand(Core::Command::Pause);
    game.update(secondsFor(3.0f));

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.score, 0);
    EXPECT_EQ(snapshot.remainingPellets, c_corridorPellets);
}

TEST(GameVictoryTest, EatingLastPelletWinsGame)
{
    Core::Game game{makeCorridorGame()};
    for (int step{0}; step < c_corridorPellets - 1; ++step)
    {
        game.update(secondsFor(1.0f));
        EXPECT_EQ(game.state(), Core::GameState::Playing) << "step " << step;
    }

    game.update(secondsFor(1.0f));
    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Victory);
    EXPECT_EQ(snapshot.remainingPellets, 0);
    EXPECT_EQ(snapshot.score, c_corridorFullScore);
}

TEST(GameVictoryTest, VictoryFreezesGame)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(3.0f));
    ASSERT_EQ(game.state(), Core::GameState::Victory);
    const double finishTime{game.totalTime()};

    game.update(1.0f);
    EXPECT_EQ(game.state(), Core::GameState::Victory);
    EXPECT_DOUBLE_EQ(game.totalTime(), finishTime);
    EXPECT_EQ(game.snapshot().score, c_corridorFullScore);

    game.handleCommand(Core::Command::Reset);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_EQ(game.snapshot().remainingPellets, c_corridorPellets);
    EXPECT_EQ(game.snapshot().highScore, c_corridorFullScore);
}

// --- Scatter and Chase waves ---

TEST(GameWaveTest, GhostModeToString)
{
    EXPECT_EQ(Core::toString(Core::GhostMode::Scatter), "Scatter");
    EXPECT_EQ(Core::toString(Core::GhostMode::Chase), "Chase");
}

TEST(GameWaveTest, SwitchesScatterChaseScatter)
{
    // The player stands still behind a wall: no pellet is eaten, no ghost reaches the player, the game stays in Playing
    Core::Game game{Tests::makePlayingGame(c_walledOffLevel)};
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    game.update(Core::c_scatterDuration - c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);

    game.update(Core::c_chaseDuration - (2.0f * c_waveMargin));
    expectAllGhostsInMode(game, Core::GhostMode::Chase);

    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    // A death would restart the wave, so the sequence above is only valid without one
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives);
}

TEST(GameWaveTest, ResetRestartsScatterWave)
{
    Core::Game game{Tests::makePlayingGame(c_walledOffLevel)};
    game.update(Core::c_scatterDuration + c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);

    game.handleCommand(Core::Command::Reset);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    // The wave timer restarts too: after the Ready pause a full Scatter wave passes before Chase
    skipReadyPhase(game);
    game.update(Core::c_scatterDuration - c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);
}

TEST(GameVictoryTest, LargeStepEatsEveryPelletOnTheWay)
{
    Core::Game game{makeCorridorGame()};
    // One frame crosses three pellet tiles; only substeps let the core see each of them
    game.update(secondsFor(3.0f));

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.remainingPellets, 0);
    EXPECT_EQ(snapshot.score, c_corridorFullScore);
    EXPECT_EQ(snapshot.state, Core::GameState::Victory);
    EXPECT_EQ(tileAt(snapshot, c_firstPellet), Core::TileType::Empty);
    EXPECT_EQ(tileAt(snapshot, c_secondPellet), Core::TileType::Empty);
    EXPECT_EQ(tileAt(snapshot, c_powerPellet), Core::TileType::Empty);
}

// --- Round phases, collisions and lives ---

TEST(GameRoundTest, RoundPhaseToString)
{
    EXPECT_EQ(Core::toString(Core::RoundPhase::Ready), "Ready");
    EXPECT_EQ(Core::toString(Core::RoundPhase::Active), "Active");
    EXPECT_EQ(Core::toString(Core::RoundPhase::PlayerDying), "PlayerDying");
}

TEST(GameRoundTest, ReadyPhaseFreezesEntitiesWhileTimeRuns)
{
    Core::Game game{Tests::makeReadyGame()};
    const Core::GameSnapshot atStart{game.snapshot()};
    EXPECT_EQ(atStart.state, Core::GameState::Playing);
    EXPECT_EQ(atStart.roundPhase, Core::RoundPhase::Ready);
    EXPECT_EQ(atStart.lives, Core::c_startingLives);
    EXPECT_FLOAT_EQ(atStart.phaseTimeLeft, Core::c_readyDuration);

    // A queued command does not start the player before the pause is over
    game.handleCommand(Core::Command::MoveRight);
    game.update(Core::c_readyDuration - 0.25f);
    const Core::GameSnapshot lateInReady{game.snapshot()};
    EXPECT_EQ(lateInReady.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(lateInReady.phaseTimeLeft, 0.25f);
    EXPECT_DOUBLE_EQ(lateInReady.totalTime, Core::c_readyDuration - 0.25f);
    expectEntitiesFrozen(atStart, lateInReady);

    game.update(0.5f);
    const Core::GameSnapshot inActive{game.snapshot()};
    EXPECT_EQ(inActive.roundPhase, Core::RoundPhase::Active);
    EXPECT_GT(inActive.player.position.x, atStart.player.position.x);
    const auto rubyIndex{static_cast<std::size_t>(Core::GhostId::Ruby)};
    EXPECT_NE(inActive.ghosts[rubyIndex].position.x, atStart.ghosts[rubyIndex].position.x);
}

TEST(GameRoundTest, GhostCollisionCostsLifeAndFreezesEntities)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(runIntoGhost(game));

    // runIntoGhost stops on the very substep of the collision, so the life is lost at once
    const Core::GameSnapshot atDeath{game.snapshot()};
    EXPECT_EQ(atDeath.state, Core::GameState::Playing);
    EXPECT_EQ(atDeath.roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_EQ(atDeath.lives, Core::c_startingLives - 1);
    EXPECT_FLOAT_EQ(atDeath.phaseTimeLeft, Core::c_dyingDuration);

    // Movement commands are ignored while the player dies
    game.handleCommand(Core::Command::MoveLeft);
    game.update(0.5f);
    const Core::GameSnapshot meanwhile{game.snapshot()};
    EXPECT_EQ(meanwhile.roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_EQ(meanwhile.lives, Core::c_startingLives - 1);
    EXPECT_NEAR(meanwhile.totalTime - atDeath.totalTime, 0.5, 1e-6);
    expectEntitiesFrozen(atDeath, meanwhile);
}

TEST(GameRoundTest, RespawnAfterDyingKeepsPelletsAndScore)
{
    const Core::Maze maze{Core::Maze::fromAscii(c_collisionLevel).value()};
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(runIntoGhost(game));
    const Core::GameSnapshot atDeath{game.snapshot()};
    ASSERT_GT(atDeath.score, 0);

    ASSERT_TRUE(runWhilePhase(game, Core::RoundPhase::PlayerDying));
    const Core::GameSnapshot respawned{game.snapshot()};
    EXPECT_EQ(respawned.state, Core::GameState::Playing);
    EXPECT_EQ(respawned.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(respawned.phaseTimeLeft, Core::c_readyDuration);
    EXPECT_EQ(respawned.lives, Core::c_startingLives - 1);
    EXPECT_EQ(respawned.score, atDeath.score);
    EXPECT_EQ(respawned.highScore, atDeath.highScore);
    EXPECT_EQ(respawned.remainingPellets, atDeath.remainingPellets);
    EXPECT_EQ(tileAt(respawned, c_collisionPelletAhead), Core::TileType::Empty);
    EXPECT_EQ(tileAt(respawned, c_collisionPelletBehind), Core::TileType::Pellet);
    expectAtSpawns(respawned, maze);
}

TEST(GameRoundTest, LosingLastLifeEndsGame)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(loseLife(game));
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives - 1);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    ASSERT_TRUE(loseLife(game));
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives - 2);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    // The last collision still plays the death pause before the game ends
    ASSERT_TRUE(runIntoGhost(game));
    EXPECT_EQ(game.snapshot().lives, 0);
    EXPECT_EQ(game.snapshot().roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    ASSERT_TRUE(runWhilePhase(game, Core::RoundPhase::PlayerDying));
    EXPECT_EQ(game.state(), Core::GameState::GameOver);
    EXPECT_EQ(game.snapshot().lives, 0);

    const double overTime{game.totalTime()};
    game.update(1.0f);
    EXPECT_DOUBLE_EQ(game.totalTime(), overTime);
}

TEST(GameRoundTest, ResetRestoresLivesAndReadyPhase)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(loseLife(game));
    ASSERT_EQ(game.snapshot().lives, Core::c_startingLives - 1);

    game.handleCommand(Core::Command::Reset);
    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_EQ(snapshot.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(snapshot.phaseTimeLeft, Core::c_readyDuration);
    EXPECT_EQ(snapshot.lives, Core::c_startingLives);
    EXPECT_EQ(snapshot.score, 0);
}
