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
using Core::Maze;
using Core::RoundPhase;

namespace
{

// Map with a single pellet right next to Chomper so Victory can be reached in one step.
constexpr std::string_view c_onePelletMap{"###########\n"
                                          "#   1     #\n"
                                          "# ##=#### #\n"
                                          "# # 234 # #\n"
                                          "###########\n"
                                          "#   P.    #\n"
                                          "###########\n"};

// Corridor where Ruby starts directly to the right of Chomper for reaching GameOver quickly.
constexpr std::string_view c_quickDeathMap{"###########\n"
                                           "#   P 1   #\n"
                                           "# ##=#### #\n"
                                           "# # 234 # #\n"
                                           "###########\n"};

Core::Game makeGameInVictory()
{
    Core::Game game{Tests::makePlayingGame(c_onePelletMap)};
    game.handleCommand(Command::MoveRight);
    game.update(1.5f / Core::c_playerSpeed);
    return game;
}

void loseOneLife(Core::Game& game)
{
    while (game.state() == GameState::Playing && game.snapshot().roundPhase == RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
    game.handleCommand(Command::MoveRight);
    for (int step{0}; step < 200 && game.snapshot().roundPhase == RoundPhase::Active; ++step)
    {
        game.update(Core::c_maxSubstep);
    }
    while (game.state() == GameState::Playing && game.snapshot().roundPhase == RoundPhase::PlayerDying)
    {
        game.update(Core::c_maxSubstep);
    }
}

Core::Game makeGameInGameOver()
{
    Core::Game game{Tests::makePlayingGame(c_quickDeathMap)};
    for (int loss{0}; loss < Core::c_startingLives; ++loss)
    {
        loseOneLife(game);
    }
    return game;
}

Core::Game makeGameInState(GameState state)
{
    switch (state)
    {
        case GameState::MainMenu: return Core::Game{Maze::fromAscii(Tests::c_smallLevel).value()};
        case GameState::Playing: return Tests::makePlayingGame();
        case GameState::Paused:
        {
            Core::Game game{Tests::makePlayingGame()};
            game.handleCommand(Command::Pause);
            return game;
        }
        case GameState::Victory: return makeGameInVictory();
        case GameState::GameOver: return makeGameInGameOver();
        case GameState::Exit:
        {
            Core::Game game{Maze::fromAscii(Tests::c_smallLevel).value()};
            game.handleCommand(Command::Quit);
            return game;
        }
    }
    return Core::Game{};
}

struct IgnoredTransition
{
    GameState state;
    Command command;
};

class IgnoredScreenTransitionsTest : public ::testing::TestWithParam<IgnoredTransition>
{
};

}  // namespace

TEST(GameScreensAndMenuAcceptance, NewStatesAndConfirmCommandHaveCanonicalNames)
{
    EXPECT_EQ(Core::toString(GameState::MainMenu), "MainMenu");
    EXPECT_EQ(Core::toString(GameState::Exit), "Exit");
    EXPECT_EQ(Core::toString(Command::Confirm), "Confirm");
}

TEST(GameScreensAndMenuAcceptance, GameStartsInMainMenuAndFreezesUntilConfirmed)
{
    Core::Game game{Maze::fromAscii(Tests::c_smallLevel).value()};
    EXPECT_EQ(game.state(), GameState::MainMenu);
    EXPECT_EQ(game.snapshot().state, GameState::MainMenu);

    game.handleCommand(Command::MoveRight);
    game.update(2.0f);
    EXPECT_EQ(game.state(), GameState::MainMenu);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);

    // Confirm starts a fresh round in Playing / Ready
    game.handleCommand(Command::Confirm);
    EXPECT_EQ(game.state(), GameState::Playing);
    EXPECT_EQ(game.snapshot().roundPhase, RoundPhase::Ready);
}

TEST(GameScreensAndMenuAcceptance, QuitFromMainMenuEntersExitAndQuitFromOtherScreensReturnsToMainMenu)
{
    Core::Game menuGame{Maze::fromAscii(Tests::c_smallLevel).value()};
    menuGame.handleCommand(Command::Quit);
    EXPECT_EQ(menuGame.state(), GameState::Exit);

    Core::Game playing{Tests::makePlayingGame()};
    playing.handleCommand(Command::Quit);
    EXPECT_EQ(playing.state(), GameState::MainMenu);

    Core::Game paused{makeGameInState(GameState::Paused)};
    paused.handleCommand(Command::Quit);
    EXPECT_EQ(paused.state(), GameState::MainMenu);

    Core::Game victory{makeGameInVictory()};
    ASSERT_EQ(victory.state(), GameState::Victory);
    victory.handleCommand(Command::Quit);
    EXPECT_EQ(victory.state(), GameState::MainMenu);

    Core::Game over{makeGameInGameOver()};
    ASSERT_EQ(over.state(), GameState::GameOver);
    over.handleCommand(Command::Quit);
    EXPECT_EQ(over.state(), GameState::MainMenu);
}

TEST(GameScreensAndMenuAcceptance, PauseResumeTogglePauseAndResetFollowTransitionTable)
{
    Core::Game game{Tests::makePlayingGame()};
    game.setHighScore(300);

    game.handleCommand(Command::Pause);
    EXPECT_EQ(game.state(), GameState::Paused);
    game.handleCommand(Command::Resume);
    EXPECT_EQ(game.state(), GameState::Playing);

    game.handleCommand(Command::TogglePause);
    EXPECT_EQ(game.state(), GameState::Paused);
    game.handleCommand(Command::TogglePause);
    EXPECT_EQ(game.state(), GameState::Playing);

    game.handleCommand(Command::Pause);
    game.handleCommand(Command::Reset);
    EXPECT_EQ(game.state(), GameState::Playing);
    EXPECT_EQ(game.snapshot().roundPhase, RoundPhase::Ready);
    EXPECT_EQ(game.snapshot().highScore, 300);
}

TEST(GameScreensAndMenuAcceptance, VictoryAndGameOverRestartOnConfirmAndResetKeepingHighScore)
{
    Core::Game victoryConfirm{makeGameInVictory()};
    ASSERT_EQ(victoryConfirm.state(), GameState::Victory);
    const int victoryHigh{victoryConfirm.snapshot().highScore};
    victoryConfirm.handleCommand(Command::Confirm);
    EXPECT_EQ(victoryConfirm.state(), GameState::Playing);
    EXPECT_EQ(victoryConfirm.snapshot().score, 0);
    EXPECT_EQ(victoryConfirm.snapshot().highScore, victoryHigh);
    EXPECT_EQ(victoryConfirm.snapshot().remainingPellets, 1);

    Core::Game victoryReset{makeGameInVictory()};
    victoryReset.handleCommand(Command::Reset);
    EXPECT_EQ(victoryReset.state(), GameState::Playing);

    Core::Game overConfirm{makeGameInGameOver()};
    ASSERT_EQ(overConfirm.state(), GameState::GameOver);
    overConfirm.handleCommand(Command::Confirm);
    EXPECT_EQ(overConfirm.state(), GameState::Playing);
    EXPECT_EQ(overConfirm.snapshot().lives, Core::c_startingLives);

    Core::Game overReset{makeGameInGameOver()};
    overReset.handleCommand(Command::Reset);
    EXPECT_EQ(overReset.state(), GameState::Playing);
}

TEST_P(IgnoredScreenTransitionsTest, IgnoredCommandLeavesStateUnchanged)
{
    const IgnoredTransition param{GetParam()};
    Core::Game game{makeGameInState(param.state)};
    ASSERT_EQ(game.state(), param.state);

    game.handleCommand(param.command);
    EXPECT_EQ(game.state(), param.state);
}

INSTANTIATE_TEST_SUITE_P(GameScreensAndMenuAcceptance, IgnoredScreenTransitionsTest,
    ::testing::Values(
        // MainMenu ignores TogglePause, Pause, Resume, Reset
        IgnoredTransition{GameState::MainMenu, Command::TogglePause}, IgnoredTransition{GameState::MainMenu, Command::Pause},
        IgnoredTransition{GameState::MainMenu, Command::Resume}, IgnoredTransition{GameState::MainMenu, Command::Reset},
        // Playing ignores Confirm, Resume
        IgnoredTransition{GameState::Playing, Command::Confirm}, IgnoredTransition{GameState::Playing, Command::Resume},
        // Paused ignores Confirm, Pause
        IgnoredTransition{GameState::Paused, Command::Confirm}, IgnoredTransition{GameState::Paused, Command::Pause},
        // Victory ignores TogglePause, Pause, Resume
        IgnoredTransition{GameState::Victory, Command::TogglePause}, IgnoredTransition{GameState::Victory, Command::Pause},
        IgnoredTransition{GameState::Victory, Command::Resume},
        // GameOver ignores TogglePause, Pause, Resume
        IgnoredTransition{GameState::GameOver, Command::TogglePause}, IgnoredTransition{GameState::GameOver, Command::Pause},
        IgnoredTransition{GameState::GameOver, Command::Resume},
        // Exit ignores every command
        IgnoredTransition{GameState::Exit, Command::Confirm}, IgnoredTransition{GameState::Exit, Command::TogglePause},
        IgnoredTransition{GameState::Exit, Command::Pause}, IgnoredTransition{GameState::Exit, Command::Resume},
        IgnoredTransition{GameState::Exit, Command::Reset}, IgnoredTransition{GameState::Exit, Command::Quit}));
