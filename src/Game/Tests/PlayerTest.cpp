#include <gtest/gtest.h>
#include "Game.h"
#include "TestHelpers.h"

using namespace LifeExe;

namespace
{

// Speed is 8 tiles per second, so these steps move the player by whole and half tiles exactly
constexpr float c_oneTileDt{0.125f};
constexpr float c_halfTileDt{0.0625f};

// Player spawn of Tests::c_smallLevel is cell {5, 5}, i.e. the point (5.5, 5.5) in tiles
constexpr float c_spawnX{5.5f};
constexpr float c_spawnY{5.5f};

void expectPosition(const Core::Game& game, float x, float y)
{
    const Core::Vec2f position{game.snapshot().player.position};
    EXPECT_FLOAT_EQ(position.x, x);
    EXPECT_FLOAT_EQ(position.y, y);
}

}  // namespace

TEST(PlayerTest, SnapshotStartsAtSpawn)
{
    const Core::Game game{Tests::makePlayingGame()};
    const Core::PlayerSnapshot player{game.snapshot().player};

    expectPosition(game, c_spawnX, c_spawnY);
    EXPECT_EQ(player.direction, Core::Direction::None);
    EXPECT_EQ(player.facing, Core::Direction::Left);
    EXPECT_FALSE(player.isMoving);
}

TEST(PlayerTest, MovesAlongStraightCorridor)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveLeft);

    game.update(c_oneTileDt);
    expectPosition(game, 4.5f, c_spawnY);
    EXPECT_EQ(game.snapshot().player.direction, Core::Direction::Left);
    EXPECT_TRUE(game.snapshot().player.isMoving);

    game.update(c_halfTileDt);
    expectPosition(game, 4.0f, c_spawnY);
}

TEST(PlayerTest, StopsAtCellCentreBeforeWall)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveDown);

    // Four tiles of travel, but the wall is only two cells below the spawn
    game.update(4.0f * c_oneTileDt);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, c_spawnX, 7.5f);
    EXPECT_FALSE(player.isMoving);
    EXPECT_EQ(player.direction, Core::Direction::None);
    EXPECT_EQ(player.facing, Core::Direction::Down);
}

TEST(PlayerTest, BufferedTurnFiresAtFirstOpenJunction)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveLeft);
    game.update(c_halfTileDt);

    // Up is blocked along row 5 until column 1, so the turn waits in the buffer
    game.handleCommand(Core::Command::MoveUp);
    game.update(c_oneTileDt);
    expectPosition(game, 4.0f, c_spawnY);
    EXPECT_EQ(game.snapshot().player.direction, Core::Direction::Left);

    // 2.5 tiles more reach the centre of column 1, where the turn happens
    game.update(2.5f * c_oneTileDt);
    expectPosition(game, 1.5f, c_spawnY);
    EXPECT_EQ(game.snapshot().player.direction, Core::Direction::Up);

    game.update(c_oneTileDt);
    expectPosition(game, 1.5f, 4.5f);
    EXPECT_EQ(game.snapshot().player.facing, Core::Direction::Up);
}

TEST(PlayerTest, ReversesInstantlyMidCellWithoutJump)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveLeft);
    game.update(c_halfTileDt);
    expectPosition(game, 5.0f, c_spawnY);

    // A zero step isolates the reversal itself: the position must not change
    game.handleCommand(Core::Command::MoveRight);
    game.update(0.0f);
    expectPosition(game, 5.0f, c_spawnY);
    EXPECT_EQ(game.snapshot().player.direction, Core::Direction::Right);
    EXPECT_EQ(game.snapshot().player.facing, Core::Direction::Right);

    game.update(c_halfTileDt);
    expectPosition(game, c_spawnX, c_spawnY);
}

TEST(PlayerTest, TunnelWrapsToOppositeSideOfRow)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveRight);

    // Six tiles: five to the right tunnel mouth {5, 10} and one through the edge to {5, 0}
    constexpr int frames{12};
    for (int frame{0}; frame < frames; ++frame)
    {
        game.update(c_halfTileDt);
        EXPECT_FLOAT_EQ(game.snapshot().player.position.y, c_spawnY);
    }

    expectPosition(game, 0.5f, c_spawnY);
    EXPECT_EQ(game.snapshot().player.direction, Core::Direction::Right);
    EXPECT_TRUE(game.snapshot().player.isMoving);
}

TEST(PlayerTest, LargeStepCrossesSeveralCellsAndTurns)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveLeft);
    // A zero step lets the player accept Left without moving
    game.update(0.0f);
    game.handleCommand(Core::Command::MoveUp);

    // One frame of six tiles: four to the left, a turn at column 1 and two up
    game.update(6.0f * c_oneTileDt);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 1.5f, 3.5f);
    EXPECT_EQ(player.direction, Core::Direction::Up);
    EXPECT_EQ(player.facing, Core::Direction::Up);
    EXPECT_TRUE(player.isMoving);
}

TEST(PlayerTest, MoveCommandsIgnoredWhilePaused)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::Pause);
    game.handleCommand(Core::Command::MoveLeft);
    game.update(4.0f * c_oneTileDt);
    expectPosition(game, c_spawnX, c_spawnY);

    // After resume the player still stands: the command was dropped, not buffered
    game.handleCommand(Core::Command::Resume);
    game.update(c_oneTileDt);
    expectPosition(game, c_spawnX, c_spawnY);
    EXPECT_EQ(game.snapshot().player.direction, Core::Direction::None);
}

TEST(PlayerTest, MoveCommandsIgnoredAfterGameOver)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::Quit);
    game.handleCommand(Core::Command::MoveLeft);
    game.update(4.0f * c_oneTileDt);

    const Core::PlayerSnapshot player{game.snapshot().player};
    EXPECT_EQ(game.snapshot().state, Core::GameState::GameOver);
    expectPosition(game, c_spawnX, c_spawnY);
    EXPECT_EQ(player.direction, Core::Direction::None);
    EXPECT_FALSE(player.isMoving);
}

TEST(PlayerTest, ResetReturnsPlayerToSpawn)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::MoveLeft);
    game.update(2.0f * c_oneTileDt);

    game.handleCommand(Core::Command::Reset);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, c_spawnX, c_spawnY);
    EXPECT_EQ(player.direction, Core::Direction::None);
    EXPECT_EQ(player.facing, Core::Direction::Left);
    EXPECT_FALSE(player.isMoving);
}
