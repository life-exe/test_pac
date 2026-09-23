#include <gtest/gtest.h>
#include "Game.h"

using namespace Game::Core;

TEST(GameCoreTest, InitialState)
{
    ::Game::Core::Game game;
    EXPECT_EQ(game.state(), GameState::Playing);
    EXPECT_FLOAT_EQ(game.totalTime(), 0.0F);
    EXPECT_EQ(game.score(), 0);
}

TEST(GameCoreTest, AccumulatesTotalTimeWhilePlaying)
{
    ::Game::Core::Game game;
    game.update(0.5F);
    game.update(0.25F);

    EXPECT_FLOAT_EQ(game.totalTime(), 0.75F);
}

TEST(GameCoreTest, PauseStopsSimulation)
{
    ::Game::Core::Game game;
    game.update(0.5F);
    game.setState(GameState::Paused);
    game.update(1.0F);

    // На паузе время симуляции не увеличивается
    EXPECT_FLOAT_EQ(game.totalTime(), 0.5F);
}

TEST(GameCoreTest, GameStateToString)
{
    EXPECT_EQ(toString(GameState::Playing), "Playing");
    EXPECT_EQ(toString(GameState::Paused), "Paused");
    EXPECT_EQ(toString(GameState::GameOver), "GameOver");
}
