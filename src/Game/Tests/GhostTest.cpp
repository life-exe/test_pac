#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "GameSnapshot.h"
#include "Ghost.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Direction;
using Core::Ghost;
using Core::GhostConfig;
using Core::GhostId;
using Core::GridPos;
using Core::Maze;

namespace
{

// Ghost speed is not a power of two, so tests stop mid-cell and never exactly on a cell boundary
constexpr float secondsFor(float tiles)
{
    return tiles / Core::c_ghostSpeed;
}

// Small step for runs that check a condition after every update
constexpr float c_smallStepDt{1.0f / 64.0f};

// T-junction at {1, 4}: a corridor along row 1 and a stem going down that ends in a dead end at {3, 4}.
// Row 5 only holds the symbols the parser requires.
constexpr std::string_view c_tJunctionLevel{"#########\n"
                                            "#       #\n"
                                            "#### ####\n"
                                            "#### ####\n"
                                            "#########\n"
                                            "#P1234=##\n"
                                            "#########\n"};

// Crossroads at {3, 4}: four open arms, so every direction is a candidate
constexpr std::string_view c_crossLevel{"#########\n"
                                        "#### ####\n"
                                        "#### ####\n"
                                        "#       #\n"
                                        "#### ####\n"
                                        "#### ####\n"
                                        "#########\n"
                                        "#P1234=##\n"
                                        "#########\n"};

Maze makeMaze(std::string_view asciiMap = Tests::c_smallLevel)
{
    return Maze::fromAscii(asciiMap).value();
}

// A ghost already outside the house, free to move at once
Ghost makeFreeGhost(GridPos spawn)
{
    return Ghost{GhostConfig{.id = GhostId::Ruby, .spawn = spawn, .scatterCorner = {0, 0}, .inHouse = false, .releaseDelay = 0.0f}};
}

Ghost makeRosie(const Maze& maze)
{
    return Ghost{GhostConfig{.id = GhostId::Rosie,
        .spawn = maze.ghostSpawn(GhostId::Rosie),
        .scatterCorner = maze.scatterCorner(GhostId::Rosie),
        .inHouse = true,
        .releaseDelay = Core::c_rosieReleaseDelay}};
}

bool isAtCentreOf(const Core::GhostSnapshot& ghost, GridPos tile)
{
    return ghost.position.x == static_cast<float>(tile.col) + 0.5f && ghost.position.y == static_cast<float>(tile.row) + 0.5f;
}

const Core::GhostSnapshot& ghostOf(const Core::GameSnapshot& snapshot, GhostId id)
{
    return snapshot.ghosts[static_cast<std::size_t>(id)];
}

void advanceTo(Core::Game& game, double time)
{
    game.update(static_cast<float>(time - game.totalTime()));
}

}  // namespace

// --- Ghost house ---

TEST(GhostTest, WaitsInHouseUntilReleaseDelay)
{
    const Maze maze{makeMaze()};
    Ghost rosie{makeRosie(maze)};

    rosie.update(maze, Core::c_rosieReleaseDelay - 0.5f);

    EXPECT_TRUE(rosie.isInHouse());
    EXPECT_EQ(rosie.tile(), maze.ghostSpawn(GhostId::Rosie));
    EXPECT_EQ(rosie.direction(), Direction::None);
    EXPECT_FLOAT_EQ(rosie.releaseDelay(), 0.5f);
}

TEST(GhostTest, LeavesHouseThroughDoorAndClearsInHouse)
{
    const Maze maze{makeMaze()};
    Ghost rosie{makeRosie(maze)};
    rosie.update(maze, Core::c_rosieReleaseDelay);
    ASSERT_TRUE(rosie.isInHouse());

    // The way out is three cells long, so a second of small steps is more than enough
    bool passedDoor{false};
    for (int step{0}; step < 64 && rosie.isInHouse(); ++step)
    {
        rosie.update(maze, c_smallStepDt);
        passedDoor = passedDoor || maze.tileAt(rosie.tile()) == Core::TileType::GhostDoor;
    }

    EXPECT_TRUE(passedDoor);
    EXPECT_FALSE(rosie.isInHouse());
    EXPECT_EQ(rosie.tile(), maze.houseExitTile());
}

TEST(GhostTest, GameReleasesGhostsInTurn)
{
    const Maze maze{makeMaze()};
    Core::Game game{Tests::makePlayingGame()};

    advanceTo(game, 0.5);
    Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_FALSE(isAtCentreOf(ghostOf(snapshot, GhostId::Ruby), maze.ghostSpawn(GhostId::Ruby)));
    EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, GhostId::Rosie), maze.ghostSpawn(GhostId::Rosie)));
    EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, GhostId::Indigo), maze.ghostSpawn(GhostId::Indigo)));
    EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, GhostId::Coral), maze.ghostSpawn(GhostId::Coral)));

    advanceTo(game, Core::c_rosieReleaseDelay - 0.5);
    EXPECT_TRUE(isAtCentreOf(ghostOf(game.snapshot(), GhostId::Rosie), maze.ghostSpawn(GhostId::Rosie)));

    advanceTo(game, Core::c_rosieReleaseDelay + 0.5);
    snapshot = game.snapshot();
    EXPECT_FALSE(isAtCentreOf(ghostOf(snapshot, GhostId::Rosie), maze.ghostSpawn(GhostId::Rosie)));
    EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, GhostId::Indigo), maze.ghostSpawn(GhostId::Indigo)));
    EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, GhostId::Coral), maze.ghostSpawn(GhostId::Coral)));

    advanceTo(game, Core::c_indigoReleaseDelay + 0.5);
    snapshot = game.snapshot();
    EXPECT_FALSE(isAtCentreOf(ghostOf(snapshot, GhostId::Indigo), maze.ghostSpawn(GhostId::Indigo)));
    EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, GhostId::Coral), maze.ghostSpawn(GhostId::Coral)));

    advanceTo(game, Core::c_coralReleaseDelay + 0.5);
    EXPECT_FALSE(isAtCentreOf(ghostOf(game.snapshot(), GhostId::Coral), maze.ghostSpawn(GhostId::Coral)));
}

TEST(GhostTest, ResetReturnsGhostsToSpawns)
{
    const Maze maze{makeMaze()};
    Core::Game game{Tests::makePlayingGame()};
    advanceTo(game, Core::c_coralReleaseDelay + 1.0);

    game.handleCommand(Core::Command::Reset);

    const Core::GameSnapshot snapshot{game.snapshot()};
    for (const GhostId id : {GhostId::Ruby, GhostId::Rosie, GhostId::Indigo, GhostId::Coral})
    {
        const Core::GhostSnapshot& ghost{ghostOf(snapshot, id)};
        EXPECT_EQ(ghost.id, id);
        EXPECT_TRUE(isAtCentreOf(ghost, maze.ghostSpawn(id)));
        EXPECT_EQ(ghost.direction, Direction::None);
    }
}

TEST(GhostTest, ResetRestartsReleaseTimers)
{
    const Maze maze{makeMaze()};
    Core::Game game{Tests::makePlayingGame()};
    advanceTo(game, Core::c_coralReleaseDelay + 1.0);

    game.handleCommand(Core::Command::Reset);
    game.update(Core::c_rosieReleaseDelay - 0.5f);

    // The timers start over, so the house ghosts wait at their spawns again
    const Core::GameSnapshot snapshot{game.snapshot()};
    for (const GhostId id : {GhostId::Rosie, GhostId::Indigo, GhostId::Coral})
    {
        EXPECT_TRUE(isAtCentreOf(ghostOf(snapshot, id), maze.ghostSpawn(id)));
    }
}

// --- Navigation ---

TEST(GhostTest, NoReverseInCorridor)
{
    const Maze maze{makeMaze(c_tJunctionLevel)};
    Ghost ghost{makeFreeGhost({1, 2})};

    ghost.updateToward(maze, secondsFor(0.5f), {1, 7});
    ASSERT_EQ(ghost.direction(), Direction::Right);

    // The target is now behind, but the next centre has no exit other than straight on
    ghost.updateToward(maze, secondsFor(1.0f), {1, 1});
    EXPECT_EQ(ghost.direction(), Direction::Right);
    EXPECT_EQ(ghost.tile(), (GridPos{1, 3}));
}

TEST(GhostTest, ReversesInDeadEnd)
{
    const Maze maze{makeMaze(c_tJunctionLevel)};
    Ghost ghost{makeFreeGhost({2, 4})};
    constexpr GridPos belowDeadEnd{9, 4};

    ghost.updateToward(maze, secondsFor(0.5f), belowDeadEnd);
    ASSERT_EQ(ghost.direction(), Direction::Down);

    ghost.updateToward(maze, secondsFor(1.0f), belowDeadEnd);
    EXPECT_EQ(ghost.direction(), Direction::Up);
    EXPECT_EQ(ghost.tile(), (GridPos{3, 4}));
}

TEST(GhostTest, TJunctionPicksShortestTurn)
{
    const Maze maze{makeMaze(c_tJunctionLevel)};

    // Two cells up the stem to the junction, then half a cell into the chosen branch
    Ghost toRight{makeFreeGhost({3, 4})};
    toRight.updateToward(maze, secondsFor(2.5f), {1, 7});
    EXPECT_EQ(toRight.tile(), (GridPos{1, 4}));
    EXPECT_EQ(toRight.direction(), Direction::Right);

    Ghost toLeft{makeFreeGhost({3, 4})};
    toLeft.updateToward(maze, secondsFor(2.5f), {1, 1});
    EXPECT_EQ(toLeft.tile(), (GridPos{1, 4}));
    EXPECT_EQ(toLeft.direction(), Direction::Left);
}

TEST(GhostTest, TiePriorityUpLeftDownRight)
{
    const Maze maze{makeMaze(c_crossLevel)};
    const Ghost ghost{makeFreeGhost({3, 4})};

    // Each target lies on a diagonal, so the two neighbours facing it are equally close
    EXPECT_EQ(ghost.chooseNextDirection(maze, {-2, -1}), Direction::Up);   // Up == Left
    EXPECT_EQ(ghost.chooseNextDirection(maze, {8, -1}), Direction::Left);  // Left == Down
    EXPECT_EQ(ghost.chooseNextDirection(maze, {8, 9}), Direction::Down);   // Down == Right
}

TEST(GhostTest, OutsideGhostNeverEntersDoor)
{
    const Maze maze{makeMaze()};
    Ghost ghost{makeFreeGhost(maze.houseExitTile())};
    const GridPos insideHouse{maze.ghostSpawn(GhostId::Indigo)};

    EXPECT_NE(ghost.chooseNextDirection(maze, insideHouse), Direction::Down);

    for (int step{0}; step < 256; ++step)
    {
        ghost.updateToward(maze, c_smallStepDt, insideHouse);
        ASSERT_NE(maze.tileAt(ghost.tile()), Core::TileType::GhostDoor) << "step " << step;
        ASSERT_FALSE(ghost.tile() == maze.houseExitTile() && ghost.direction() == Direction::Down) << "step " << step;
    }
}

TEST(GhostTest, PassesThroughTunnelWithoutReversing)
{
    const Maze maze{makeMaze()};
    Ghost ghost{makeFreeGhost({5, 8})};
    constexpr GridPos beyondRightEdge{5, 20};

    // Four and a half cells: {5, 8} -> {5, 10} -> jump to {5, 0} -> half a cell past {5, 1}
    bool sawTunnelExit{false};
    for (int step{0}; step < 36; ++step)
    {
        ghost.updateToward(maze, secondsFor(0.125f), beyondRightEdge);
        ASSERT_NE(ghost.direction(), Direction::Left) << "step " << step;
        sawTunnelExit = sawTunnelExit || ghost.tile() == GridPos{5, 0};
    }

    EXPECT_TRUE(sawTunnelExit);
    EXPECT_EQ(ghost.tile(), (GridPos{5, 1}));
    EXPECT_EQ(ghost.direction(), Direction::Right);
}
