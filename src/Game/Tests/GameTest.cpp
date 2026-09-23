#include <gtest/gtest.h>
#include "Command.h"
#include "Game.h"

using namespace LifeExe;

TEST(GameCoreTest, InitialState)
{
    const Core::Game game;
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);
    EXPECT_EQ(game.score(), 0);
}

TEST(GameCoreTest, AccumulatesTotalTimeWhilePlaying)
{
    Core::Game game;
    game.update(0.5f);
    game.update(0.25f);

    EXPECT_DOUBLE_EQ(game.totalTime(), 0.75);
}

TEST(GameCoreTest, PauseStopsSimulation)
{
    Core::Game game;
    game.update(0.5f);
    game.setState(Core::GameState::Paused);
    game.update(1.0f);

    EXPECT_DOUBLE_EQ(game.totalTime(), 0.5);
}

TEST(GameCoreTest, GameStateToString)
{
    EXPECT_EQ(Core::toString(Core::GameState::Playing), "Playing");
    EXPECT_EQ(Core::toString(Core::GameState::Paused), "Paused");
    EXPECT_EQ(Core::toString(Core::GameState::GameOver), "GameOver");
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

TEST(GameCoreTest, HandlesResetCommand)
{
    Core::Game game;
    game.update(1.5f);
    game.handleCommand(Core::Command::Pause);

    game.handleCommand(Core::Command::Reset);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);
    EXPECT_EQ(game.score(), 0);
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
    EXPECT_EQ(Core::toString(Core::Command::Reset), "Reset");
    EXPECT_EQ(Core::toString(Core::Command::Quit), "Quit");
}
