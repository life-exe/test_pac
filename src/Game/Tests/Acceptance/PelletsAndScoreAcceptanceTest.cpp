#include <gtest/gtest.h>

#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::GameState;
using Core::TileType;

namespace
{

constexpr float secondsFor(float tiles)
{
    return tiles / Core::c_playerSpeed;
}

// Small corridor map with 3 regular pellets and 1 power pellet on row 5
constexpr std::string_view c_pelletMap{"###########\n"
                                       "#   1     #\n"
                                       "#   =     #\n"
                                       "#  234    #\n"
                                       "#         #\n"
                                       "T   P..o. T\n"
                                       "###########\n"};

// Map with zero pellets to verify Victory is only triggered on eating the last pellet
constexpr std::string_view c_emptyPelletMap{"###########\n"
                                            "#   1     #\n"
                                            "#   =     #\n"
                                            "#  234    #\n"
                                            "#         #\n"
                                            "T   P     T\n"
                                            "###########\n"};

}  // namespace

TEST(PelletsAndScoreAcceptance, GameStateVictoryHasCanonicalName)
{
    EXPECT_EQ(Core::toString(GameState::Victory), "Victory");
}

TEST(PelletsAndScoreAcceptance, MazeCountsAndConsumesPellets)
{
    Core::Maze maze{Core::Maze::fromAscii(c_pelletMap).value()};
    EXPECT_EQ(maze.remainingPellets(), 4);

    EXPECT_EQ(maze.consumeTileAt({5, 5}), TileType::Pellet);
    EXPECT_EQ(maze.tileAt({5, 5}), TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 3);

    // Consuming an already eaten cell does nothing
    EXPECT_EQ(maze.consumeTileAt({5, 5}), TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 3);

    EXPECT_EQ(maze.consumeTileAt({5, 7}), TileType::PowerPellet);
    EXPECT_EQ(maze.tileAt({5, 7}), TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 2);

    // Walls and out-of-bounds cells cannot be consumed
    EXPECT_EQ(maze.consumeTileAt({0, 0}), TileType::Empty);
    EXPECT_EQ(maze.consumeTileAt({-1, 0}), TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 2);
}

TEST(PelletsAndScoreAcceptance, EatingPelletsAndPowerPelletsUpdatesScoreAndHighScore)
{
    Core::Game game{Tests::makePlayingGame(c_pelletMap)};
    EXPECT_EQ(game.snapshot().score, 0);
    EXPECT_EQ(game.snapshot().highScore, 0);
    EXPECT_EQ(game.snapshot().remainingPellets, 4);

    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(1.2f));
    EXPECT_EQ(game.snapshot().score, Core::c_pelletScore);
    EXPECT_EQ(game.snapshot().highScore, Core::c_pelletScore);
    EXPECT_EQ(game.snapshot().remainingPellets, 3);

    // Move across the second pellet and the power pellet
    game.update(secondsFor(2.0f));
    const int expected{(2 * Core::c_pelletScore) + Core::c_powerPelletScore};
    EXPECT_EQ(game.snapshot().score, expected);
    EXPECT_EQ(game.snapshot().highScore, expected);
    EXPECT_EQ(game.snapshot().remainingPellets, 1);
    EXPECT_EQ(game.state(), GameState::Playing);
}

TEST(PelletsAndScoreAcceptance, LargeDtSubsteppingEatsAllPelletsAlongThePath)
{
    Core::Game game{Tests::makePlayingGame(c_pelletMap)};
    game.handleCommand(Command::MoveRight);

    // One large dt covering 4 cells must eat all 3 pellets and 1 power pellet without skipping any
    game.update(secondsFor(4.5f));

    const int expected{(3 * Core::c_pelletScore) + Core::c_powerPelletScore};
    EXPECT_EQ(game.snapshot().score, expected);
    EXPECT_EQ(game.snapshot().remainingPellets, 0);
    EXPECT_EQ(game.state(), GameState::Victory);
}

TEST(PelletsAndScoreAcceptance, ResetRestoresMazeAndScoreButKeepsHighScore)
{
    Core::Game game{Tests::makePlayingGame(c_pelletMap)};
    game.setHighScore(200);
    EXPECT_EQ(game.snapshot().highScore, 200);

    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(4.5f));
    EXPECT_EQ(game.state(), GameState::Victory);

    game.handleCommand(Command::Reset);
    const Core::GameSnapshot snap{game.snapshot()};
    EXPECT_EQ(snap.state, GameState::Playing);
    EXPECT_EQ(snap.score, 0);
    EXPECT_EQ(snap.highScore, 200);
    EXPECT_EQ(snap.remainingPellets, 4);
}

TEST(PelletsAndScoreAcceptance, MapWithoutPelletsDoesNotTriggerVictoryImmediately)
{
    Core::Game game{Tests::makePlayingGame(c_emptyPelletMap)};
    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(2.0f));

    EXPECT_EQ(game.state(), GameState::Playing);
    EXPECT_EQ(game.snapshot().remainingPellets, 0);
}

TEST(PelletsAndScoreAcceptance, VictoryFreezesMovementAndIgnoresMoveCommands)
{
    Core::Game game{Tests::makePlayingGame(c_pelletMap)};
    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(4.5f));
    ASSERT_EQ(game.state(), GameState::Victory);

    const Core::Vec2f victoryPos{game.snapshot().player.position};
    game.handleCommand(Command::MoveLeft);
    game.update(1.0f);

    EXPECT_EQ(game.state(), GameState::Victory);
    EXPECT_FLOAT_EQ(game.snapshot().player.position.x, victoryPos.x);
    EXPECT_FLOAT_EQ(game.snapshot().player.position.y, victoryPos.y);
}
