#include <gtest/gtest.h>
#include "Game.h"

using namespace LifeExe;

TEST(GameCoreTest, InitialState)
{
    const Core::Game game;
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_FLOAT_EQ(game.totalTime(), 0.0f);
}

TEST(GameCoreTest, AccumulatesTotalTimeWhilePlaying)
{
    Core::Game game;
    game.update(0.5f);
    game.update(0.25f);

    EXPECT_FLOAT_EQ(game.totalTime(), 0.75f);
}

TEST(GameCoreTest, SnapshotCopiesCoreState)
{
    Core::Game game;
    game.update(0.5f);

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_FLOAT_EQ(snapshot.totalTime, 0.5f);
}

TEST(GameCoreTest, GameStateToString)
{
    EXPECT_EQ(Core::toString(Core::GameState::Playing), "Playing");
    EXPECT_EQ(Core::toString(Core::GameState::Paused), "Paused");
    EXPECT_EQ(Core::toString(Core::GameState::GameOver), "GameOver");
}
