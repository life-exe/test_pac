#include <gtest/gtest.h>

#include "GhostStrategy.h"
#include "Types.h"

using namespace LifeExe;
using Core::Direction;
using Core::GhostContext;
using Core::GridPos;

namespace
{

// Fields a strategy must ignore get distinct values, so reading the wrong one changes the result
constexpr GridPos c_noiseRuby{20, 21};
constexpr GridPos c_noiseGhost{22, 23};
constexpr GridPos c_corner{-1, 27};

GhostContext makeContext(GridPos playerPos, Direction playerDir)
{
    return {.playerPos = playerPos, .playerDir = playerDir, .rubyPos = c_noiseRuby, .ghostPos = c_noiseGhost, .scatterCorner = c_corner};
}

GhostContext makeIndigoContext(GridPos playerPos, Direction playerDir, GridPos rubyPos)
{
    GhostContext context{makeContext(playerPos, playerDir)};
    context.rubyPos = rubyPos;
    return context;
}

// Coral stands at the origin, so the player position alone sets the squared distance
GhostContext makeCoralContext(GridPos playerPos)
{
    GhostContext context{makeContext(playerPos, Direction::Right)};
    context.ghostPos = {0, 0};
    return context;
}

}  // namespace

// --- Ruby: direct chase ---

TEST(RubyStrategyTest, TargetsPlayerTile)
{
    EXPECT_EQ(Core::rubyStrategy(makeContext({10, 10}, Direction::Up)), (GridPos{10, 10}));
    EXPECT_EQ(Core::rubyStrategy(makeContext({1, 1}, Direction::Left)), (GridPos{1, 1}));
    EXPECT_EQ(Core::rubyStrategy(makeContext({-2, 30}, Direction::None)), (GridPos{-2, 30}));
}

// --- Rosie: four tiles ahead of the player ---

TEST(RosieStrategyTest, TargetsFourTilesAhead)
{
    EXPECT_EQ(Core::rosieStrategy(makeContext({10, 10}, Direction::Up)), (GridPos{6, 10}));
    EXPECT_EQ(Core::rosieStrategy(makeContext({10, 10}, Direction::Down)), (GridPos{14, 10}));
    EXPECT_EQ(Core::rosieStrategy(makeContext({10, 10}, Direction::Left)), (GridPos{10, 6}));
    EXPECT_EQ(Core::rosieStrategy(makeContext({10, 10}, Direction::Right)), (GridPos{10, 14}));
}

TEST(RosieStrategyTest, NoneDirectionTargetsPlayer)
{
    EXPECT_EQ(Core::rosieStrategy(makeContext({10, 10}, Direction::None)), (GridPos{10, 10}));
}

TEST(RosieStrategyTest, TargetMayLeaveMap)
{
    EXPECT_EQ(Core::rosieStrategy(makeContext({1, 1}, Direction::Up)), (GridPos{-3, 1}));
    EXPECT_EQ(Core::rosieStrategy(makeContext({1, 1}, Direction::Left)), (GridPos{1, -3}));
}

// --- Indigo: doubled vector from Ruby to the pivot two tiles ahead of the player ---

TEST(IndigoStrategyTest, DoublesVectorFromRubyToPivot)
{
    constexpr GridPos ruby{8, 8};
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({10, 10}, Direction::Up, ruby)), (GridPos{8, 12}));
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({10, 10}, Direction::Down, ruby)), (GridPos{16, 12}));
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({10, 10}, Direction::Left, ruby)), (GridPos{12, 8}));
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({10, 10}, Direction::Right, ruby)), (GridPos{12, 16}));
}

TEST(IndigoStrategyTest, NoneDirectionUsesPlayerAsPivot)
{
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({10, 10}, Direction::None, {8, 8})), (GridPos{12, 12}));
}

TEST(IndigoStrategyTest, RubyOnPivotTargetsPivot)
{
    // Pivot is {10, 12}: two tiles to the right of the player
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({10, 10}, Direction::Right, {10, 12})), (GridPos{10, 12}));
}

TEST(IndigoStrategyTest, TargetMayLeaveMap)
{
    EXPECT_EQ(Core::indigoStrategy(makeIndigoContext({1, 1}, Direction::Up, {5, 5})), (GridPos{-7, -3}));
}

// --- Coral: chases from afar, retreats to its corner within 8 tiles ---

TEST(CoralStrategyTest, ChasesWhenFartherThanEightTiles)
{
    // distSq = 8 * 8 + 1 * 1 = 65
    EXPECT_EQ(Core::coralStrategy(makeCoralContext({8, 1})), (GridPos{8, 1}));
}

TEST(CoralStrategyTest, ExactBoundaryRetreatsToCorner)
{
    // distSq = 8 * 8 = 64: the threshold is strict, so the boundary counts as close
    EXPECT_EQ(Core::coralStrategy(makeCoralContext({8, 0})), c_corner);
}

TEST(CoralStrategyTest, RetreatsWhenClose)
{
    // distSq = 3 * 3 + 4 * 4 = 25
    EXPECT_EQ(Core::coralStrategy(makeCoralContext({3, 4})), c_corner);
}
