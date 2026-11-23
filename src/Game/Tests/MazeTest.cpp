#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Game.h"
#include "Level.h"
#include "Maze.h"
#include "TestHelpers.h"

using namespace LifeExe;

namespace
{

Core::Maze parseOrFail(std::string_view asciiMap)
{
    return Core::Maze::fromAscii(asciiMap).value();
}

// Every perimeter cell must be a wall, except the listed tunnel cells
void expectClosedPerimeter(const Core::Maze& maze, std::span<const Core::GridPos> tunnels)
{
    const auto expectBorderCell = [&](Core::GridPos pos)
    {
        const bool isTunnel{std::ranges::find(tunnels, pos) != tunnels.end()};
        const Core::TileType expected{isTunnel ? Core::TileType::Tunnel : Core::TileType::Wall};
        EXPECT_EQ(maze.tileAt(pos), expected) << "row " << pos.row << ", col " << pos.col;
    };

    for (int col{0}; col < maze.cols(); ++col)
    {
        expectBorderCell({0, col});
        expectBorderCell({maze.rows() - 1, col});
    }
    for (int row{1}; row < maze.rows() - 1; ++row)
    {
        expectBorderCell({row, 0});
        expectBorderCell({row, maze.cols() - 1});
    }
}

void expectError(std::string_view asciiMap, Core::MazeError expected)
{
    const auto result{Core::Maze::fromAscii(asciiMap)};
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), expected);
}

constexpr std::array c_allMovers{Core::Mover::Player, Core::Mover::Ghost, Core::Mover::GhostUsingDoor};

}  // namespace

// --- Grid geometry ---

TEST(MazeTest, SmallLevelHasExpectedSize)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    EXPECT_EQ(maze.rows(), 9);
    EXPECT_EQ(maze.cols(), 11);
    EXPECT_EQ(maze.tiles().size(), std::size_t{99});
}

TEST(MazeTest, ClassicLevelHasExpectedSize)
{
    const Core::Maze maze{parseOrFail(Core::c_classicLevel)};
    EXPECT_EQ(maze.rows(), 21);
    EXPECT_EQ(maze.cols(), 21);
    EXPECT_EQ(maze.tiles().size(), std::size_t{441});
}

TEST(MazeTest, SmallLevelPerimeterIsWallExceptTunnels)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr std::array tunnels{Core::GridPos{5, 0}, Core::GridPos{5, 10}};
    expectClosedPerimeter(maze, tunnels);
}

TEST(MazeTest, ClassicLevelPerimeterIsWallExceptTunnels)
{
    const Core::Maze maze{parseOrFail(Core::c_classicLevel)};
    constexpr std::array tunnels{Core::GridPos{9, 0}, Core::GridPos{9, 20}};
    expectClosedPerimeter(maze, tunnels);
}

// --- Walkability ---

TEST(MazeTest, GhostDoorIsWalkableOnlyForGhostUsingDoor)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr Core::GridPos door{2, 5};
    ASSERT_EQ(maze.tileAt(door), Core::TileType::GhostDoor);

    EXPECT_FALSE(maze.isWalkable(door, Core::Mover::Player));
    EXPECT_FALSE(maze.isWalkable(door, Core::Mover::Ghost));
    EXPECT_TRUE(maze.isWalkable(door, Core::Mover::GhostUsingDoor));
}

TEST(MazeTest, WallIsNotWalkableForAnyMover)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr Core::GridPos wall{0, 0};
    ASSERT_EQ(maze.tileAt(wall), Core::TileType::Wall);

    for (const Core::Mover mover : c_allMovers)
    {
        EXPECT_FALSE(maze.isWalkable(wall, mover));
    }
}

TEST(MazeTest, OpenTilesAreWalkableForAllMovers)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr std::array openTiles{
        std::pair{Core::GridPos{1, 1}, Core::TileType::Pellet},
        std::pair{Core::GridPos{6, 1}, Core::TileType::PowerPellet},
        std::pair{Core::GridPos{3, 3}, Core::TileType::Empty},
        std::pair{Core::GridPos{5, 0}, Core::TileType::Tunnel},
    };

    for (const auto& [pos, type] : openTiles)
    {
        ASSERT_EQ(maze.tileAt(pos), type);
        for (const Core::Mover mover : c_allMovers)
        {
            EXPECT_TRUE(maze.isWalkable(pos, mover)) << "row " << pos.row << ", col " << pos.col;
        }
    }
}

// --- Spawns ---

TEST(MazeTest, SpawnPositionsMatchMap)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    EXPECT_EQ(maze.playerSpawn(), (Core::GridPos{5, 5}));
    EXPECT_EQ(maze.ghostSpawn(Core::GhostId::Ruby), (Core::GridPos{1, 5}));
    EXPECT_EQ(maze.ghostSpawn(Core::GhostId::Rosie), (Core::GridPos{3, 4}));
    EXPECT_EQ(maze.ghostSpawn(Core::GhostId::Indigo), (Core::GridPos{3, 5}));
    EXPECT_EQ(maze.ghostSpawn(Core::GhostId::Coral), (Core::GridPos{3, 6}));
}

TEST(MazeTest, SpawnTilesAreEmpty)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    EXPECT_EQ(maze.tileAt(maze.playerSpawn()), Core::TileType::Empty);
    for (const Core::GhostId id : {Core::GhostId::Ruby, Core::GhostId::Rosie, Core::GhostId::Indigo, Core::GhostId::Coral})
    {
        EXPECT_EQ(maze.tileAt(maze.ghostSpawn(id)), Core::TileType::Empty);
    }
}

// --- Bounds ---

TEST(MazeTest, OutOfBoundsIsWall)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    const std::array outside{
        Core::GridPos{-1, 0},
        Core::GridPos{0, -1},
        Core::GridPos{maze.rows(), 0},
        Core::GridPos{0, maze.cols()},
        Core::GridPos{-100, -100},
        Core::GridPos{1000, 1000},
    };

    for (const Core::GridPos pos : outside)
    {
        EXPECT_EQ(maze.tileAt(pos), Core::TileType::Wall) << "row " << pos.row << ", col " << pos.col;
        for (const Core::Mover mover : c_allMovers)
        {
            EXPECT_FALSE(maze.isWalkable(pos, mover));
        }
    }
}

// --- Parse errors: each map differs from c_smallLevel in one spot ---

TEST(MazeErrorTest, EmptyMap)
{
    expectError("", Core::MazeError::EmptyMap);
    expectError("\n", Core::MazeError::EmptyMap);
}

TEST(MazeErrorTest, RaggedRows)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#........#\n"
                "###########\n",
        Core::MazeError::RaggedRows);
}

TEST(MazeErrorTest, UnknownSymbol)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#....X....#\n"
                "###########\n",
        Core::MazeError::UnknownSymbol);
}

TEST(MazeErrorTest, MissingPlayerSpawn)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "T.........T\n"
                "#o.......o#\n"
                "#.........#\n"
                "###########\n",
        Core::MazeError::MissingPlayerSpawn);
}

TEST(MazeErrorTest, MissingGhostSpawn)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 23  #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#.........#\n"
                "###########\n",
        Core::MazeError::MissingGhostSpawn);
}

TEST(MazeErrorTest, DuplicatePlayerSpawn)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#....P....#\n"
                "###########\n",
        Core::MazeError::DuplicateSpawn);
}

TEST(MazeErrorTest, DuplicateGhostSpawn)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 232 #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#.........#\n"
                "###########\n",
        Core::MazeError::DuplicateSpawn);
}

TEST(MazeErrorTest, MissingGhostDoor)
{
    expectError("###########\n"
                "#....1....#\n"
                "#.#######.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#.........#\n"
                "###########\n",
        Core::MazeError::MissingGhostDoor);
}

TEST(MazeErrorTest, UnpairedTunnel)
{
    // Only one edge of the row has a tunnel
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "#....P....T\n"
                "#o.......o#\n"
                "#.........#\n"
                "###########\n",
        Core::MazeError::UnpairedTunnel);

    // A tunnel away from the edge cannot lead anywhere
    expectError("###########\n"
                "#....1....#\n"
                "#.###=###.#\n"
                "#.# 234 #.#\n"
                "#.#######.#\n"
                "T....P....T\n"
                "#o.......o#\n"
                "#....T....#\n"
                "###########\n",
        Core::MazeError::UnpairedTunnel);
}

// --- Pellets ---

TEST(MazeTest, SmallLevelCountsPellets)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    // 38 regular pellets and 2 power pellets
    EXPECT_EQ(maze.remainingPellets(), 40);
}

TEST(MazeTest, ClassicLevelCountsPellets)
{
    const Core::Maze maze{parseOrFail(Core::c_classicLevel)};
    // Counting tiles directly gives an oracle that does not depend on the parser's counter
    const auto pellets{std::ranges::count_if(
        maze.tiles(), [](Core::TileType tile) { return tile == Core::TileType::Pellet || tile == Core::TileType::PowerPellet; })};
    EXPECT_GT(maze.remainingPellets(), 0);
    EXPECT_EQ(maze.remainingPellets(), static_cast<int>(pellets));
}

TEST(MazeTest, ConsumePelletReturnsPelletAndClearsTile)
{
    Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr Core::GridPos pellet{1, 1};
    ASSERT_EQ(maze.tileAt(pellet), Core::TileType::Pellet);

    EXPECT_EQ(maze.consumeTileAt(pellet), Core::TileType::Pellet);
    EXPECT_EQ(maze.tileAt(pellet), Core::TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 39);
}

TEST(MazeTest, ConsumePowerPelletReturnsPowerPellet)
{
    Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr Core::GridPos powerPellet{6, 1};
    ASSERT_EQ(maze.tileAt(powerPellet), Core::TileType::PowerPellet);

    EXPECT_EQ(maze.consumeTileAt(powerPellet), Core::TileType::PowerPellet);
    EXPECT_EQ(maze.tileAt(powerPellet), Core::TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 39);
}

TEST(MazeTest, ConsumeClearedTileIsNoOp)
{
    Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr Core::GridPos pellet{1, 1};
    ASSERT_EQ(maze.consumeTileAt(pellet), Core::TileType::Pellet);

    EXPECT_EQ(maze.consumeTileAt(pellet), Core::TileType::Empty);
    EXPECT_EQ(maze.tileAt(pellet), Core::TileType::Empty);
    EXPECT_EQ(maze.remainingPellets(), 39);
}

TEST(MazeTest, ConsumeWallAndEmptyTileIsNoOp)
{
    Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    constexpr std::array untouched{
        std::pair{Core::GridPos{0, 0}, Core::TileType::Wall},
        std::pair{Core::GridPos{3, 3}, Core::TileType::Empty},
        std::pair{Core::GridPos{5, 0}, Core::TileType::Tunnel},
        std::pair{Core::GridPos{2, 5}, Core::TileType::GhostDoor},
    };

    for (const auto& [pos, type] : untouched)
    {
        ASSERT_EQ(maze.tileAt(pos), type);
        EXPECT_EQ(maze.consumeTileAt(pos), Core::TileType::Empty) << "row " << pos.row << ", col " << pos.col;
        // The tile keeps its type: only pellets are replaced
        EXPECT_EQ(maze.tileAt(pos), type) << "row " << pos.row << ", col " << pos.col;
    }
    EXPECT_EQ(maze.remainingPellets(), 40);
}

TEST(MazeTest, ConsumeOutOfBoundsIsNoOp)
{
    Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    const std::array outside{
        Core::GridPos{-1, 0},
        Core::GridPos{0, -1},
        Core::GridPos{maze.rows(), 0},
        Core::GridPos{0, maze.cols()},
    };

    for (const Core::GridPos pos : outside)
    {
        EXPECT_EQ(maze.consumeTileAt(pos), Core::TileType::Empty) << "row " << pos.row << ", col " << pos.col;
    }
    EXPECT_EQ(maze.remainingPellets(), 40);
}

// --- Game owns the maze ---

TEST(GameMazeTest, DefaultGameUsesClassicLevel)
{
    const Core::Game game;
    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.mazeRows, 21);
    EXPECT_EQ(snapshot.mazeCols, 21);
    EXPECT_EQ(snapshot.tiles.size(), std::size_t{441});
}

TEST(GameMazeTest, SnapshotExposesInjectedMaze)
{
    const Core::Maze maze{parseOrFail(Tests::c_smallLevel)};
    const Core::Game game{Tests::makePlayingGame()};
    const Core::GameSnapshot snapshot{game.snapshot()};

    ASSERT_EQ(snapshot.mazeRows, maze.rows());
    ASSERT_EQ(snapshot.mazeCols, maze.cols());
    ASSERT_EQ(snapshot.tiles.size(), maze.tiles().size());
    for (int row{0}; row < maze.rows(); ++row)
    {
        for (int col{0}; col < maze.cols(); ++col)
        {
            const std::size_t index{
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(snapshot.mazeCols)) + static_cast<std::size_t>(col)};
            EXPECT_EQ(snapshot.tiles[index], maze.tileAt({row, col})) << "row " << row << ", col " << col;
        }
    }
}

TEST(GameMazeTest, GameIsMovable)
{
    static_assert(std::is_move_constructible_v<Core::Game>);
    static_assert(std::is_move_assignable_v<Core::Game>);

    Core::Game source{Tests::makePlayingGame()};
    const Core::Game moved{std::move(source)};
    EXPECT_EQ(moved.snapshot().mazeRows, 9);
    EXPECT_EQ(moved.snapshot().mazeCols, 11);

    Core::Game assigned;
    assigned = Tests::makePlayingGame();
    EXPECT_EQ(assigned.snapshot().mazeRows, 9);
    EXPECT_EQ(assigned.snapshot().mazeCols, 11);
}
