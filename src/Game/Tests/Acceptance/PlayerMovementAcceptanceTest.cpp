#include <gtest/gtest.h>

#include <cmath>
#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "GridMover.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::Direction;
using Core::GridPos;
using Core::Mover;

namespace
{

constexpr float c_tolerance{1e-3f};

// The time the player needs to cover the given number of tiles
constexpr float secondsFor(float tiles)
{
    return tiles / Core::c_playerSpeed;
}

void expectPosition(const Core::Game& game, float x, float y)
{
    const Core::PlayerSnapshot player{game.snapshot().player};
    EXPECT_NEAR(player.position.x, x, c_tolerance);
    EXPECT_NEAR(player.position.y, y, c_tolerance);
}

}  // namespace

TEST(PlayerMovementAcceptance, StartsAtRestInTheCentreOfTheSpawnCell)
{
    Core::Game game{Tests::makePlayingGame()};
    game.update(1.0f);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 5.5f, 5.5f);
    EXPECT_FALSE(player.isMoving);
    EXPECT_EQ(player.direction, Direction::None);
    EXPECT_EQ(player.facing, Direction::Left);
}

TEST(PlayerMovementAcceptance, MovesAlongAStraightCorridor)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(2.5f));

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 8.0f, 5.5f);
    EXPECT_TRUE(player.isMoving);
    EXPECT_EQ(player.direction, Direction::Right);
    EXPECT_EQ(player.facing, Direction::Right);
}

TEST(PlayerMovementAcceptance, WallAheadAtRestKeepsThePlayerStill)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveUp);
    game.update(1.0f);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 5.5f, 5.5f);
    EXPECT_FALSE(player.isMoving);
    EXPECT_EQ(player.direction, Direction::None);
    EXPECT_EQ(player.facing, Direction::Left);
}

TEST(PlayerMovementAcceptance, StopsInTheCentreOfTheCellBeforeAWall)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveDown);
    game.update(1.0f);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 5.5f, 7.5f);
    EXPECT_FALSE(player.isMoving);
    EXPECT_EQ(player.direction, Direction::None);
    EXPECT_EQ(player.facing, Direction::Down);
}

TEST(PlayerMovementAcceptance, BufferedTurnIsAppliedAtTheFirstOpenCrossing)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);
    game.update(secondsFor(0.1f));
    game.handleCommand(Command::MoveUp);
    game.update(secondsFor(4.9f));

    // The turn up is blocked by walls above columns 2..4, so it happens in column 1
    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 1.5f, 4.5f);
    EXPECT_EQ(player.direction, Direction::Up);
    EXPECT_EQ(player.facing, Direction::Up);
}

TEST(PlayerMovementAcceptance, ResultDoesNotDependOnStepSize)
{
    Core::Game bigStep{Tests::makePlayingGame()};
    bigStep.handleCommand(Command::MoveLeft);
    bigStep.update(secondsFor(0.1f));
    bigStep.handleCommand(Command::MoveUp);
    bigStep.update(secondsFor(4.9f));

    Core::Game smallSteps{Tests::makePlayingGame()};
    smallSteps.handleCommand(Command::MoveLeft);
    smallSteps.update(secondsFor(0.1f));
    smallSteps.handleCommand(Command::MoveUp);
    for (int step{0}; step < 49; ++step)
    {
        smallSteps.update(secondsFor(0.1f));
    }

    const Core::Vec2f expected{bigStep.snapshot().player.position};
    expectPosition(smallSteps, expected.x, expected.y);
    EXPECT_EQ(smallSteps.snapshot().player.direction, bigStep.snapshot().player.direction);
}

TEST(PlayerMovementAcceptance, ReversalBetweenCellCentresIsInstantAndKeepsThePosition)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);
    game.update(secondsFor(0.5f));
    expectPosition(game, 5.0f, 5.5f);

    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(0.4f));

    // 0.5 tile back to the cell centre plus 0.4 further, no jump
    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 5.4f, 5.5f);
    EXPECT_EQ(player.direction, Direction::Right);
    EXPECT_EQ(player.facing, Direction::Right);
}

TEST(PlayerMovementAcceptance, TunnelMovesThePlayerToTheOtherSideOfTheRow)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);
    game.update(secondsFor(6.5f));

    // 5 tiles to the tunnel cell, 1 tile beyond the edge, then 0.5 tile from the right tunnel cell
    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 10.0f, 5.5f);
    EXPECT_EQ(player.direction, Direction::Left);
}

TEST(PlayerMovementAcceptance, TunnelJumpHappensOncePerPass)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);

    int jumps{0};
    float previousX{game.snapshot().player.position.x};
    for (int step{0}; step < 200; ++step)
    {
        game.update(0.01f);
        const float x{game.snapshot().player.position.x};
        if (std::abs(x - previousX) > 5.0f)
        {
            ++jumps;
        }
        previousX = x;
    }
    EXPECT_EQ(jumps, 1);
}

TEST(PlayerMovementAcceptance, ReversalWaitsForTheTunnelJumpBeyondTheEdge)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);
    game.update(secondsFor(5.5f));
    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(0.1f));

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, -0.1f, 5.5f);
    EXPECT_EQ(player.direction, Direction::Left);
}

TEST(PlayerMovementAcceptance, MoveCommandsAreIgnoredOutsidePlaying)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::Pause);
    game.handleCommand(Command::MoveLeft);
    game.handleCommand(Command::Resume);
    game.update(secondsFor(2.0f));

    expectPosition(game, 5.5f, 5.5f);
    EXPECT_FALSE(game.snapshot().player.isMoving);
}

TEST(PlayerMovementAcceptance, PauseFreezesThePlayer)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);
    game.update(secondsFor(0.5f));
    game.handleCommand(Command::Pause);
    game.update(secondsFor(2.0f));

    expectPosition(game, 5.0f, 5.5f);
}

TEST(PlayerMovementAcceptance, ResetReturnsThePlayerToTheSpawn)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveLeft);
    game.update(secondsFor(2.0f));
    game.handleCommand(Command::Reset);

    const Core::PlayerSnapshot player{game.snapshot().player};
    expectPosition(game, 5.5f, 5.5f);
    EXPECT_FALSE(player.isMoving);
    EXPECT_EQ(player.direction, Direction::None);
}

TEST(PlayerMovementAcceptance, PlayerSpeedIsPositive)
{
    EXPECT_GT(Core::c_playerSpeed, 0.0f);
}

TEST(PlayerMovementAcceptance, OppositeSwapsDirections)
{
    EXPECT_EQ(Core::opposite(Direction::Up), Direction::Down);
    EXPECT_EQ(Core::opposite(Direction::Down), Direction::Up);
    EXPECT_EQ(Core::opposite(Direction::Left), Direction::Right);
    EXPECT_EQ(Core::opposite(Direction::Right), Direction::Left);
    EXPECT_EQ(Core::opposite(Direction::None), Direction::None);
}

TEST(PlayerMovementAcceptance, DirectionDeltaIsAnOffsetInRowsAndColumns)
{
    EXPECT_EQ(Core::directionDelta(Direction::Up), (GridPos{-1, 0}));
    EXPECT_EQ(Core::directionDelta(Direction::Down), (GridPos{1, 0}));
    EXPECT_EQ(Core::directionDelta(Direction::Left), (GridPos{0, -1}));
    EXPECT_EQ(Core::directionDelta(Direction::Right), (GridPos{0, 1}));
    EXPECT_EQ(Core::directionDelta(Direction::None), (GridPos{0, 0}));
}

TEST(PlayerMovementAcceptance, CanEnterRespectsWallsDoorsAndTunnels)
{
    const Core::Maze maze{Core::Maze::fromAscii(Tests::c_smallLevel).value()};

    EXPECT_TRUE(Core::canEnter(maze, Mover::Player, {5, 5}, Direction::Left));
    EXPECT_FALSE(Core::canEnter(maze, Mover::Player, {5, 5}, Direction::Up));
    EXPECT_FALSE(Core::canEnter(maze, Mover::Player, {5, 5}, Direction::None));
    EXPECT_FALSE(Core::canEnter(maze, Mover::Player, {6, 1}, Direction::Left));

    EXPECT_FALSE(Core::canEnter(maze, Mover::Player, {1, 5}, Direction::Down));
    EXPECT_FALSE(Core::canEnter(maze, Mover::Ghost, {1, 5}, Direction::Down));
    EXPECT_TRUE(Core::canEnter(maze, Mover::GhostUsingDoor, {1, 5}, Direction::Down));

    // The tunnel mouth is the only place where a mover may leave the map
    EXPECT_TRUE(Core::canEnter(maze, Mover::Player, {5, 0}, Direction::Left));
    EXPECT_TRUE(Core::canEnter(maze, Mover::Player, {5, 10}, Direction::Right));
    EXPECT_TRUE(Core::canEnter(maze, Mover::Ghost, {5, 0}, Direction::Left));
}

TEST(PlayerMovementAcceptance, MoveCommandsHaveNames)
{
    EXPECT_EQ(Core::toString(Command::MoveUp), "MoveUp");
    EXPECT_EQ(Core::toString(Command::MoveDown), "MoveDown");
    EXPECT_EQ(Core::toString(Command::MoveLeft), "MoveLeft");
    EXPECT_EQ(Core::toString(Command::MoveRight), "MoveRight");
}
