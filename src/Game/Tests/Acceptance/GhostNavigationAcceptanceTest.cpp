#include <gtest/gtest.h>

#include <cmath>
#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "Ghost.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::Direction;
using Core::Ghost;
using Core::GhostConfig;
using Core::GhostId;
using Core::GridPos;
using Core::Maze;

namespace
{

constexpr float c_tolerance{1e-3f};

constexpr float ghostSecondsFor(float tiles)
{
    return tiles / Core::c_ghostSpeed;
}

Core::Vec2f cellCentre(GridPos pos)
{
    return {static_cast<float>(pos.col) + 0.5f, static_cast<float>(pos.row) + 0.5f};
}

void expectAtCellCentre(Core::Vec2f actual, GridPos cell)
{
    const Core::Vec2f expected{cellCentre(cell)};
    EXPECT_NEAR(actual.x, expected.x, c_tolerance);
    EXPECT_NEAR(actual.y, expected.y, c_tolerance);
}

void expectGhostsAtSpawns(const Maze& maze, const Core::GameSnapshot& snap, std::size_t firstIndex)
{
    for (std::size_t i{firstIndex}; i < snap.ghosts.size(); ++i)
    {
        expectAtCellCentre(snap.ghosts[i].position, maze.ghostSpawn(snap.ghosts[i].id));
    }
}

// Straight horizontal corridor with a dead-end at (1, 7) and a T-junction at (1, 3)
constexpr std::string_view c_junctionMap{"#########\n"
                                         "#1......#\n"
                                         "###.###=#\n"
                                         "#P#.#234#\n"
                                         "#########\n"};

// Open 4-way cross at (2, 2) for tie-breaking priority checks
constexpr std::string_view c_crossMap{"#########\n"
                                      "##.####1#\n"
                                      "#...###=#\n"
                                      "##.#P234#\n"
                                      "#########\n"};

// Straight tunnel row on row 3
constexpr std::string_view c_tunnelMap{"#########\n"
                                       "#  1=234#\n"
                                       "####.####\n"
                                       "T.......T\n"
                                       "#   P   #\n"
                                       "#########\n"};

}  // namespace

TEST(GhostNavigationAcceptance, GhostConstantsArePositiveAndOrdered)
{
    EXPECT_GT(Core::c_ghostSpeed, 0.0f);
    EXPECT_GT(Core::c_rosieReleaseDelay, 0.0f);
    EXPECT_GT(Core::c_indigoReleaseDelay, Core::c_rosieReleaseDelay);
    EXPECT_GT(Core::c_coralReleaseDelay, Core::c_indigoReleaseDelay);
}

TEST(GhostNavigationAcceptance, SnapshotContainsAllFourGhostsAtSpawns)
{
    const Maze maze{Maze::fromAscii(Tests::c_smallLevel).value()};
    const Core::Game game{Tests::makePlayingGame()};
    const Core::GameSnapshot snap{game.snapshot()};

    EXPECT_EQ(snap.ghosts[0].id, GhostId::Ruby);
    EXPECT_EQ(snap.ghosts[1].id, GhostId::Rosie);
    EXPECT_EQ(snap.ghosts[2].id, GhostId::Indigo);
    EXPECT_EQ(snap.ghosts[3].id, GhostId::Coral);
    expectGhostsAtSpawns(maze, snap, 0);
}

TEST(GhostNavigationAcceptance, RubyMovesImmediatelyWhileHouseGhostsWaitForReleaseDelays)
{
    const Maze maze{Maze::fromAscii(Tests::c_smallLevel).value()};
    Core::Game game{Tests::makePlayingGame()};

    game.update(1.0f);
    const Core::GameSnapshot afterOneSec{game.snapshot()};

    const Core::Vec2f rubySpawn{cellCentre(maze.ghostSpawn(GhostId::Ruby))};
    const float rubyShift{
        std::abs(afterOneSec.ghosts[0].position.x - rubySpawn.x) + std::abs(afterOneSec.ghosts[0].position.y - rubySpawn.y)};
    EXPECT_GT(rubyShift, 0.5f);

    // Rosie, Indigo and Coral are still waiting inside the house
    expectGhostsAtSpawns(maze, afterOneSec, 1);

    // After Rosie's delay plus exit travel time (3 cells out through the door + 0.5 cell into row 1),
    // Rosie is outside above the door while Coral is still waiting at its spawn
    game.update((Core::c_rosieReleaseDelay - 1.0f) + ghostSecondsFor(3.5f));
    const Core::GameSnapshot afterRosie{game.snapshot()};
    EXPECT_LT(afterRosie.ghosts[1].position.y, 2.0f);
    expectAtCellCentre(afterRosie.ghosts[3].position, maze.ghostSpawn(GhostId::Coral));
}

TEST(GhostNavigationAcceptance, GhostLeavesHouseThroughDoorAndClearsInHouseFlag)
{
    const Maze maze{Maze::fromAscii(Tests::c_smallLevel).value()};
    Ghost rosie{{.id = GhostId::Rosie,
        .spawn = maze.ghostSpawn(GhostId::Rosie),
        .scatterCorner = maze.scatterCorner(GhostId::Rosie),
        .inHouse = true,
        .releaseDelay = Core::c_rosieReleaseDelay}};

    EXPECT_TRUE(rosie.isInHouse());
    rosie.update(maze, Core::c_rosieReleaseDelay - 0.5f);
    EXPECT_TRUE(rosie.isInHouse());
    EXPECT_EQ(rosie.tile(), maze.ghostSpawn(GhostId::Rosie));

    // Remaining 0.5s delay plus enough time to walk 3 tiles out through the door
    rosie.update(maze, 0.5f + ghostSecondsFor(4.0f));
    EXPECT_FALSE(rosie.isInHouse());
    EXPECT_LT(rosie.tile().row, 2);
}

TEST(GhostNavigationAcceptance, ChooseNextDirectionForbids180DegreeReversalInCorridor)
{
    const Maze maze{Maze::fromAscii(c_junctionMap).value()};
    Ghost ghost{{.id = GhostId::Ruby, .spawn = {1, 1}, .scatterCorner = {1, 7}, .inHouse = false, .releaseDelay = 0.0f}};

    // Move one cell right so current direction is Right at (1, 2)
    ghost.updateToward(maze, ghostSecondsFor(1.0f), {1, 7});
    ASSERT_EQ(ghost.tile(), (GridPos{1, 2}));
    ASSERT_EQ(ghost.direction(), Direction::Right);

    // Even with a target behind on the left, the ghost must continue right
    EXPECT_EQ(ghost.chooseNextDirection(maze, {1, 0}), Direction::Right);
}

TEST(GhostNavigationAcceptance, ChooseNextDirectionReversesInDeadEnd)
{
    const Maze maze{Maze::fromAscii(c_junctionMap).value()};
    Ghost ghost{{.id = GhostId::Ruby, .spawn = {1, 6}, .scatterCorner = {1, 7}, .inHouse = false, .releaseDelay = 0.0f}};

    // Reach dead-end cell (1, 7) moving Right: with walls on Up, Down and Right, the ghost must reverse Left immediately
    ghost.updateToward(maze, ghostSecondsFor(1.0f), {1, 8});
    ASSERT_EQ(ghost.tile(), (GridPos{1, 7}));
    EXPECT_EQ(ghost.direction(), Direction::Left);

    // One more tile confirms the ghost actually leaves the dead-end instead of stalling
    ghost.updateToward(maze, ghostSecondsFor(1.0f), {1, 8});
    EXPECT_EQ(ghost.tile(), (GridPos{1, 6}));
    EXPECT_EQ(ghost.direction(), Direction::Left);
}

TEST(GhostNavigationAcceptance, ChooseNextDirectionPicksShortestEuclideanDistanceAtJunction)
{
    const Maze maze{Maze::fromAscii(c_junctionMap).value()};
    Ghost ghost{{.id = GhostId::Ruby, .spawn = {1, 2}, .scatterCorner = {1, 7}, .inHouse = false, .releaseDelay = 0.0f}};

    // Arrive at T-junction (1, 3) moving Right; open exits (excluding Left) are Down (2, 3) and Right (1, 4)
    ghost.updateToward(maze, ghostSecondsFor(1.0f), {1, 7});
    ASSERT_EQ(ghost.tile(), (GridPos{1, 3}));
    ASSERT_EQ(ghost.direction(), Direction::Right);

    EXPECT_EQ(ghost.chooseNextDirection(maze, {5, 3}), Direction::Down);
    EXPECT_EQ(ghost.chooseNextDirection(maze, {1, 10}), Direction::Right);
}

TEST(GhostNavigationAcceptance, ChooseNextDirectionBreaksTiesWithUpLeftDownRightPriority)
{
    const Maze maze{Maze::fromAscii(c_crossMap).value()};
    // At rest in centre (2, 2) of a 4-way cross: all four directions are open and none is opposite
    const Ghost ghost{{.id = GhostId::Ruby, .spawn = {2, 2}, .scatterCorner = {0, 0}, .inHouse = false, .releaseDelay = 0.0f}};

    // Target (2, 2): all 4 neighbours have squared distance 1 -> Up wins
    EXPECT_EQ(ghost.chooseNextDirection(maze, {2, 2}), Direction::Up);
    // Target (0, 0): Up (1, 2) distSq=5, Left (2, 1) distSq=5 -> Up wins over Left
    EXPECT_EQ(ghost.chooseNextDirection(maze, {0, 0}), Direction::Up);
    // Target (4, 0): Left (2, 1) distSq=5, Down (3, 2) distSq=5 -> Left wins over Down
    EXPECT_EQ(ghost.chooseNextDirection(maze, {4, 0}), Direction::Left);
    // Target (4, 4): Down (3, 2) distSq=5, Right (2, 3) distSq=5 -> Down wins over Right
    EXPECT_EQ(ghost.chooseNextDirection(maze, {4, 4}), Direction::Down);
}

TEST(GhostNavigationAcceptance, GhostInMazeDoesNotReenterGhostDoor)
{
    const Maze maze{Maze::fromAscii(Tests::c_smallLevel).value()};
    const Ghost ruby{{.id = GhostId::Ruby,
        .spawn = maze.houseExitTile(),
        .scatterCorner = maze.scatterCorner(GhostId::Ruby),
        .inHouse = false,
        .releaseDelay = 0.0f}};

    // Even when targeting the cell directly inside the house below the door, Ruby must not enter '='
    const GridPos insideHouse{maze.houseExitTile().row + 2, maze.houseExitTile().col};
    const Direction chosen{ruby.chooseNextDirection(maze, insideHouse)};
    EXPECT_NE(chosen, Direction::Down);
    EXPECT_NE(chosen, Direction::None);
}

TEST(GhostNavigationAcceptance, GhostTraversesTunnelWithoutReversing)
{
    const Maze maze{Maze::fromAscii(c_tunnelMap).value()};
    Ghost ghost{{.id = GhostId::Ruby, .spawn = {3, 2}, .scatterCorner = {3, -5}, .inHouse = false, .releaseDelay = 0.0f}};

    // Move left through (3, 1) -> (3, 0) -> tunnel exit (3, 8) -> (3, 7)
    ghost.updateToward(maze, ghostSecondsFor(4.0f), {3, -5});
    EXPECT_EQ(ghost.tile(), (GridPos{3, 7}));
    EXPECT_EQ(ghost.direction(), Direction::Left);
}

TEST(GhostNavigationAcceptance, ResetRestoresAllGhostsToSpawnsAndReleaseDelays)
{
    const Maze maze{Maze::fromAscii(Tests::c_smallLevel).value()};
    Core::Game game{Tests::makePlayingGame()};

    game.update(Core::c_coralReleaseDelay + 2.0f);
    game.handleCommand(Command::Reset);
    expectGhostsAtSpawns(maze, game.snapshot(), 0);

    // House release timers must be restored: after 1s Rosie/Indigo/Coral are still waiting in the house
    game.update(1.0f);
    expectGhostsAtSpawns(maze, game.snapshot(), 1);
}
