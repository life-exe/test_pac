#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

#include "Game.h"
#include "GameConstants.h"
#include "TestHelpers.h"

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
