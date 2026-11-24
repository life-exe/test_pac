#include <gtest/gtest.h>

#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "Ghost.h"
#include "GhostStrategy.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::coralStrategy;
using Core::Direction;
using Core::Ghost;
using Core::GhostContext;
using Core::GhostId;
using Core::GhostMode;
using Core::GhostStrategy;
using Core::GridPos;
using Core::indigoStrategy;
using Core::Maze;
using Core::rosieStrategy;
using Core::rubyStrategy;

namespace
{

constexpr float c_tolerance{1e-3f};

constexpr float ghostSecondsFor(float tiles)
{
    return tiles / Core::c_ghostSpeed;
}

void expectAllGhostsInMode(const Core::GameSnapshot& snap, GhostMode expected)
{
    for (const Core::GhostSnapshot& ghost : snap.ghosts)
    {
        EXPECT_EQ(ghost.mode, expected);
    }
}

// Corridor with a T-junction at (1, 3): exits Down (2, 3) and Right (1, 4)
constexpr std::string_view c_junctionMap{"#########\n"
                                         "#1......#\n"
                                         "###.###=#\n"
                                         "#P#.#234#\n"
                                         "#########\n"};

// Player is walled off from ghosts so wave timer checks stay independent of
// collisions
constexpr std::string_view c_waveMap{"###########\n"
                                     "#....1....#\n"
                                     "#.###=###.#\n"
                                     "#.# 234 #.#\n"
                                     "###########\n"
                                     "T....P....T\n"
                                     "###########\n"};

}  // namespace

TEST(GhostAiStrategyAcceptance, GhostModeToStringHasCanonicalNames)
{
    EXPECT_EQ(Core::toString(GhostMode::Scatter), "Scatter");
    EXPECT_EQ(Core::toString(GhostMode::Chase), "Chase");
}

TEST(GhostAiStrategyAcceptance, WaveDurationConstantsArePositive)
{
    EXPECT_GT(Core::c_scatterDuration, 0.0f);
    EXPECT_GT(Core::c_chaseDuration, 0.0f);
}

TEST(GhostAiStrategyAcceptance, RubyStrategyTargetsPlayerDirectly)
{
    const GhostStrategy strategy{rubyStrategy};
    const GhostContext context{
        .playerPos = {10, 7}, .playerDir = Direction::Left, .rubyPos = {2, 2}, .ghostPos = {2, 2}, .scatterCorner = {-3, 18}};

    EXPECT_EQ(strategy(context), (GridPos{10, 7}));
}

TEST(GhostAiStrategyAcceptance, RosieStrategyTargetsFourTilesAheadWithoutLegacyOverflow)
{
    const GhostStrategy strategy{rosieStrategy};
    const GhostContext base{.playerPos = {10, 10}, .rubyPos = {1, 1}, .ghostPos = {2, 2}, .scatterCorner = {-3, 2}};

    auto withDir = [&](Direction dir)
    {
        GhostContext ctx{base};
        ctx.playerDir = dir;
        return strategy(ctx);
    };

    EXPECT_EQ(withDir(Direction::Up), (GridPos{6, 10}));
    EXPECT_EQ(withDir(Direction::Down), (GridPos{14, 10}));
    EXPECT_EQ(withDir(Direction::Left), (GridPos{10, 6}));
    EXPECT_EQ(withDir(Direction::Right), (GridPos{10, 14}));
    EXPECT_EQ(withDir(Direction::None), (GridPos{10, 10}));
}

TEST(GhostAiStrategyAcceptance, IndigoStrategyDoublesVectorFromRubyToPivotTwoTilesAhead)
{
    const GhostStrategy strategy{indigoStrategy};

    // Pivot is (10, 12); vector from Ruby (10, 6) to pivot is (0, 6); doubled ->
    // (10, 18)
    const GhostContext horizontal{
        .playerPos = {10, 10}, .playerDir = Direction::Right, .rubyPos = {10, 6}, .ghostPos = {5, 5}, .scatterCorner = {22, 20}};
    EXPECT_EQ(strategy(horizontal), (GridPos{10, 18}));

    // Target may legitimately fall outside map bounds: pivot (3, 5), ruby (7, 6)
    // -> delta (-4, -1) -> (-1, 4)
    const GhostContext outOfBounds{
        .playerPos = {5, 5}, .playerDir = Direction::Up, .rubyPos = {7, 6}, .ghostPos = {8, 8}, .scatterCorner = {22, 20}};
    EXPECT_EQ(strategy(outOfBounds), (GridPos{-1, 4}));

    // Direction::None uses playerPos itself as the pivot
    const GhostContext stopped{
        .playerPos = {5, 5}, .playerDir = Direction::None, .rubyPos = {3, 4}, .ghostPos = {8, 8}, .scatterCorner = {22, 20}};
    EXPECT_EQ(strategy(stopped), (GridPos{7, 6}));
}

TEST(GhostAiStrategyAcceptance, CoralStrategyChasesBeyondEightTilesAndRetreatsAtOrWithinEight)
{
    const GhostStrategy strategy{coralStrategy};
    const GridPos corner{22, 0};

    // Distance squared = 65 (> 64): chases player
    const GhostContext farContext{
        .playerPos = {10, 10}, .playerDir = Direction::Left, .rubyPos = {1, 1}, .ghostPos = {18, 11}, .scatterCorner = corner};
    EXPECT_EQ(strategy(farContext), (GridPos{10, 10}));

    // Exact boundary: distance squared = 64 (not > 64): retreats to scatter
    // corner
    const GhostContext boundaryContext{
        .playerPos = {10, 10}, .playerDir = Direction::Left, .rubyPos = {1, 1}, .ghostPos = {18, 10}, .scatterCorner = corner};
    EXPECT_EQ(strategy(boundaryContext), corner);

    // Distance squared = 25 (<= 64): retreats to scatter corner
    const GhostContext closeContext{
        .playerPos = {10, 10}, .playerDir = Direction::Left, .rubyPos = {1, 1}, .ghostPos = {13, 14}, .scatterCorner = corner};
    EXPECT_EQ(strategy(closeContext), corner);
}

TEST(GhostAiStrategyAcceptance, GameAlternatesScatterAndChaseOnWaveTimerAndResets)
{
    Core::Game game{Tests::makePlayingGame(c_waveMap)};
    expectAllGhostsInMode(game.snapshot(), GhostMode::Scatter);

    game.update(Core::c_scatterDuration - 0.5f);
    expectAllGhostsInMode(game.snapshot(), GhostMode::Scatter);

    // Crossing c_scatterDuration switches all ghosts to Chase
    game.update(0.5f);
    expectAllGhostsInMode(game.snapshot(), GhostMode::Chase);

    // Crossing c_chaseDuration switches all ghosts back to Scatter
    game.update(Core::c_chaseDuration);
    expectAllGhostsInMode(game.snapshot(), GhostMode::Scatter);

    // Move back into Chase and verify Reset restores Scatter and wave timer
    game.update(Core::c_scatterDuration);
    ASSERT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Chase);
    game.handleCommand(Command::Reset);
    expectAllGhostsInMode(game.snapshot(), GhostMode::Scatter);

    game.update(1.0f);
    EXPECT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Scatter);
}

TEST(GhostAiStrategyAcceptance, ModeChangeReversesGhostInMazeWithoutJumpingPosition)
{
    const Maze maze{Maze::fromAscii(c_junctionMap).value()};
    Ghost ruby{{.id = GhostId::Ruby, .spawn = {1, 1}, .scatterCorner = {1, 7}, .inHouse = false, .releaseDelay = 0.0f}, rubyStrategy};

    // Move 0.5 tile right from (1, 1) -> position (2.0, 1.5), direction Right
    ruby.update(maze, ghostSecondsFor(0.5f));
    ASSERT_EQ(ruby.direction(), Direction::Right);
    const Core::Vec2f beforeReverse{ruby.position()};

    ruby.setMode(GhostMode::Chase, maze);
    EXPECT_EQ(ruby.mode(), GhostMode::Chase);
    EXPECT_EQ(ruby.direction(), Direction::Left);
    EXPECT_NEAR(ruby.position().x, beforeReverse.x, c_tolerance);
    EXPECT_NEAR(ruby.position().y, beforeReverse.y, c_tolerance);
}

TEST(GhostAiStrategyAcceptance, ModeChangeDoesNotReverseGhostInsideHouse)
{
    const Maze smallMaze{Maze::fromAscii(Tests::c_smallLevel).value()};
    Ghost indigo{{.id = GhostId::Indigo,
                     .spawn = smallMaze.ghostSpawn(GhostId::Indigo),
                     .scatterCorner = smallMaze.scatterCorner(GhostId::Indigo),
                     .inHouse = true,
                     .releaseDelay = 0.0f},
        indigoStrategy};
    indigo.update(smallMaze, ghostSecondsFor(0.5f));
    ASSERT_TRUE(indigo.isInHouse());
    ASSERT_EQ(indigo.direction(), Direction::Up);

    indigo.setMode(GhostMode::Chase, smallMaze);
    EXPECT_EQ(indigo.mode(), GhostMode::Chase);
    EXPECT_EQ(indigo.direction(), Direction::Up);
}

TEST(GhostAiStrategyAcceptance, GhostFollowsScatterCornerInScatterAndStrategyInChase)
{
    const Maze maze{Maze::fromAscii(c_junctionMap).value()};
    const GhostContext context{.playerPos = {3, 1}, .playerDir = Direction::None, .rubyPos = {1, 1}, .ghostPos = {1, 3}};
    const auto fixedTarget = [](const GhostContext& /*context*/) { return GridPos{6, 3}; };

    // In Scatter, scatterCorner (1, 10) pulls the ghost Right past junction (1,
    // 3)
    Ghost scatterGhost{
        {.id = GhostId::Ruby, .spawn = {1, 2}, .scatterCorner = {1, 10}, .inHouse = false, .releaseDelay = 0.0f}, fixedTarget};
    scatterGhost.update(maze, ghostSecondsFor(2.0f), context);
    EXPECT_EQ(scatterGhost.tile(), (GridPos{1, 4}));

    // In Chase, fixedTarget targets (6, 3) and pulls the ghost Down at junction (1, 3)
    Ghost chaseGhost{{.id = GhostId::Ruby, .spawn = {1, 2}, .scatterCorner = {1, 10}, .inHouse = false, .releaseDelay = 0.0f}, fixedTarget};
    chaseGhost.setMode(GhostMode::Chase, maze);
    chaseGhost.update(maze, ghostSecondsFor(2.0f), context);
    EXPECT_EQ(chaseGhost.tile(), (GridPos{2, 3}));
}

TEST(GhostAiStrategyAcceptance, ModeChangeAtJunctionCentreReverses180DegreesFromEntryDirection)
{
    const Maze maze{Maze::fromAscii(c_junctionMap).value()};
    const GhostContext context{.playerPos = {1, 7}, .playerDir = Direction::None, .rubyPos = {1, 3}, .ghostPos = {1, 3}};
    const auto pullRight = [](const GhostContext& /*ctx*/) { return GridPos{1, 7}; };

    // Start in the vertical branch (2, 3) and arrive at T-junction centre (1, 3) moving Up.
    // At the exact cell centre decideAtCentre already picks Right toward scatterCorner (1, 7).
    Ghost ruby{{.id = GhostId::Ruby, .spawn = {2, 3}, .scatterCorner = {1, 7}, .inHouse = false, .releaseDelay = 0.0f}, pullRight};
    ruby.update(maze, ghostSecondsFor(1.0f), context);
    ASSERT_EQ(ruby.tile(), (GridPos{1, 3}));
    ASSERT_EQ(ruby.direction(), Direction::Right);

    // Reversal must invert the entry heading (Up -> Down), not the pre-selected turn (Right -> Left),
    // and the next step must actually move Down instead of snapping back Right toward the Chase target.
    ruby.setMode(GhostMode::Chase, maze);
    EXPECT_EQ(ruby.direction(), Direction::Down);

    ruby.update(maze, ghostSecondsFor(1.0f), context);
    EXPECT_EQ(ruby.tile(), (GridPos{2, 3}));
    EXPECT_EQ(ruby.direction(), Direction::Down);
}
