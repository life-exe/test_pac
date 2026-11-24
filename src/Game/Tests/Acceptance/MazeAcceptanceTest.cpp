#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <type_traits>

#include "Game.h"
#include "Level.h"
#include "Maze.h"
#include "TestHelpers.h"

using namespace LifeExe;
using Core::GhostId;
using Core::GridPos;
using Core::MazeError;
using Core::Mover;
using Core::TileType;

namespace
{

// Rows of c_smallLevel are 11 characters long plus the line terminator
constexpr std::size_t c_smallLevelStride{12};

std::size_t cellOffset(int row, int col, std::size_t stride)
{
    return (static_cast<std::size_t>(row) * stride) + static_cast<std::size_t>(col);
}

// Returns the small level with one symbol replaced
std::string withSymbol(GridPos pos, char symbol)
{
    std::string text{Tests::c_smallLevel};
    text[cellOffset(pos.row, pos.col, c_smallLevelStride)] = symbol;
    return text;
}

Core::Maze smallMaze()
{
    return Core::Maze::fromAscii(Tests::c_smallLevel).value();
}

void expectError(const std::string& map, MazeError expected)
{
    const auto maze{Core::Maze::fromAscii(map)};
    ASSERT_FALSE(maze.has_value());
    EXPECT_EQ(maze.error(), expected);
}

void expectClosedOuterBorder(std::string_view map)
{
    const Core::Maze maze{Core::Maze::fromAscii(map).value()};
    for (int row{0}; row < maze.rows(); ++row)
    {
        for (const int col : {0, maze.cols() - 1})
        {
            const TileType tile{maze.tileAt({row, col})};
            EXPECT_TRUE(tile == TileType::Wall || tile == TileType::Tunnel) << "row " << row << ", column " << col;
        }
    }
    for (int col{0}; col < maze.cols(); ++col)
    {
        EXPECT_EQ(maze.tileAt({0, col}), TileType::Wall);
        EXPECT_EQ(maze.tileAt({maze.rows() - 1, col}), TileType::Wall);
    }
}

}  // namespace

TEST(MazeAcceptance, ParsesDimensions)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_EQ(maze.rows(), 9);
    EXPECT_EQ(maze.cols(), 11);
    EXPECT_EQ(maze.tiles().size(), 99U);
}

TEST(MazeAcceptance, MapsLegendSymbolsToTiles)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_EQ(maze.tileAt({0, 0}), TileType::Wall);
    EXPECT_EQ(maze.tileAt({1, 1}), TileType::Pellet);
    EXPECT_EQ(maze.tileAt({6, 1}), TileType::PowerPellet);
    EXPECT_EQ(maze.tileAt({2, 5}), TileType::GhostDoor);
    EXPECT_EQ(maze.tileAt({5, 0}), TileType::Tunnel);
    EXPECT_EQ(maze.tileAt({5, 10}), TileType::Tunnel);
    EXPECT_EQ(maze.tileAt({3, 3}), TileType::Empty);
}

TEST(MazeAcceptance, SpawnCellsAreEmptyTiles)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_EQ(maze.tileAt({5, 5}), TileType::Empty);
    EXPECT_EQ(maze.tileAt({1, 5}), TileType::Empty);
    EXPECT_EQ(maze.tileAt({3, 4}), TileType::Empty);
    EXPECT_EQ(maze.tileAt({3, 5}), TileType::Empty);
    EXPECT_EQ(maze.tileAt({3, 6}), TileType::Empty);
}

TEST(MazeAcceptance, ReportsSpawnsAndHouseExit)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_EQ(maze.playerSpawn(), (GridPos{5, 5}));
    EXPECT_EQ(maze.ghostSpawn(GhostId::Ruby), (GridPos{1, 5}));
    EXPECT_EQ(maze.ghostSpawn(GhostId::Rosie), (GridPos{3, 4}));
    EXPECT_EQ(maze.ghostSpawn(GhostId::Indigo), (GridPos{3, 5}));
    EXPECT_EQ(maze.ghostSpawn(GhostId::Coral), (GridPos{3, 6}));
    EXPECT_EQ(maze.houseExitTile(), (GridPos{1, 5}));
}

TEST(MazeAcceptance, TunnelsAreLinkedWithinTheirRow)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_EQ(maze.tunnelExit({5, 0}), (GridPos{5, 10}));
    EXPECT_EQ(maze.tunnelExit({5, 10}), (GridPos{5, 0}));
}

TEST(MazeAcceptance, ScatterCornersLieOutsideTheMap)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_EQ(maze.scatterCorner(GhostId::Ruby), (GridPos{-3, 8}));
    EXPECT_EQ(maze.scatterCorner(GhostId::Rosie), (GridPos{-3, 2}));
    EXPECT_EQ(maze.scatterCorner(GhostId::Indigo), (GridPos{10, 10}));
    EXPECT_EQ(maze.scatterCorner(GhostId::Coral), (GridPos{10, 0}));
}

TEST(MazeAcceptance, WallsBlockEveryMover)
{
    const Core::Maze maze{smallMaze()};
    for (const Mover mover : {Mover::Player, Mover::Ghost, Mover::GhostUsingDoor})
    {
        EXPECT_FALSE(maze.isWalkable({0, 0}, mover));
        EXPECT_TRUE(maze.isWalkable({1, 1}, mover));
        EXPECT_TRUE(maze.isWalkable({5, 0}, mover));
    }
}

TEST(MazeAcceptance, GhostDoorIsOpenOnlyForGhostUsingDoor)
{
    const Core::Maze maze{smallMaze()};
    EXPECT_FALSE(maze.isWalkable({2, 5}, Mover::Player));
    EXPECT_FALSE(maze.isWalkable({2, 5}, Mover::Ghost));
    EXPECT_TRUE(maze.isWalkable({2, 5}, Mover::GhostUsingDoor));
}

TEST(MazeAcceptance, CellsOutsideTheMapAreWalls)
{
    const Core::Maze maze{smallMaze()};
    for (const GridPos pos : {GridPos{-1, 0}, GridPos{0, -1}, GridPos{9, 0}, GridPos{0, 11}, GridPos{-3, 8}, GridPos{10, 10}})
    {
        EXPECT_EQ(maze.tileAt(pos), TileType::Wall);
        EXPECT_FALSE(maze.isWalkable(pos, Mover::GhostUsingDoor));
    }
}

TEST(MazeAcceptance, OuterBorderIsClosedExceptForTunnels)
{
    expectClosedOuterBorder(Tests::c_smallLevel);
    expectClosedOuterBorder(Core::c_classicLevel);
}

TEST(MazeAcceptance, ClassicLevelIsValid)
{
    const auto maze{Core::Maze::fromAscii(Core::c_classicLevel)};
    ASSERT_TRUE(maze.has_value());
    EXPECT_EQ(maze->tileAt(maze->playerSpawn()), TileType::Empty);
    EXPECT_NE(maze->houseExitTile(), (GridPos{}));
}

TEST(MazeAcceptance, RejectsEmptyMap)
{
    expectError("", MazeError::EmptyMap);
    expectError("\n", MazeError::EmptyMap);
}

TEST(MazeAcceptance, RejectsRaggedRows)
{
    std::string text{Tests::c_smallLevel};
    text.erase(cellOffset(3, 0, c_smallLevelStride), 1);
    expectError(text, MazeError::RaggedRows);
}

TEST(MazeAcceptance, RejectsUnknownSymbol)
{
    expectError(withSymbol({7, 3}, 'X'), MazeError::UnknownSymbol);
}

TEST(MazeAcceptance, RejectsMissingPlayerSpawn)
{
    expectError(withSymbol({5, 5}, '.'), MazeError::MissingPlayerSpawn);
}

TEST(MazeAcceptance, RejectsMissingGhostSpawn)
{
    expectError(withSymbol({3, 6}, ' '), MazeError::MissingGhostSpawn);
}

TEST(MazeAcceptance, RejectsDuplicatePlayerSpawn)
{
    expectError(withSymbol({7, 3}, 'P'), MazeError::DuplicateSpawn);
}

TEST(MazeAcceptance, RejectsDuplicateGhostSpawn)
{
    expectError(withSymbol({7, 3}, '2'), MazeError::DuplicateSpawn);
}

TEST(MazeAcceptance, RejectsMissingGhostDoor)
{
    expectError(withSymbol({2, 5}, '#'), MazeError::MissingGhostDoor);
}

TEST(MazeAcceptance, RejectsSingleTunnel)
{
    expectError(withSymbol({5, 10}, '#'), MazeError::UnpairedTunnel);
}

TEST(MazeAcceptance, RejectsTunnelsInDifferentRows)
{
    std::string text{withSymbol({5, 10}, '#')};
    text[cellOffset(6, 10, c_smallLevelStride)] = 'T';
    expectError(text, MazeError::UnpairedTunnel);
}

TEST(MazeAcceptance, RejectsTunnelInsideTheRow)
{
    expectError(withSymbol({5, 3}, 'T'), MazeError::UnpairedTunnel);
}

TEST(MazeAcceptance, AcceptsMapWithoutTrailingNewline)
{
    std::string text{Tests::c_smallLevel};
    text.pop_back();
    EXPECT_TRUE(Core::Maze::fromAscii(text).has_value());
}

TEST(MazeAcceptance, SnapshotExposesTheMaze)
{
    const Core::Game game{Core::Maze::fromAscii(Tests::c_smallLevel).value()};
    const Core::GameSnapshot snapshot{game.snapshot()};
    const Core::Maze maze{smallMaze()};

    EXPECT_EQ(snapshot.mazeRows, 9);
    EXPECT_EQ(snapshot.mazeCols, 11);
    ASSERT_EQ(snapshot.tiles.size(), 99U);
    const auto stride{static_cast<std::size_t>(snapshot.mazeCols)};
    for (int row{0}; row < snapshot.mazeRows; ++row)
    {
        for (int col{0}; col < snapshot.mazeCols; ++col)
        {
            EXPECT_EQ(snapshot.tiles[cellOffset(row, col, stride)], maze.tileAt({row, col}));
        }
    }
}

TEST(MazeAcceptance, DefaultGameUsesTheClassicLevel)
{
    const Core::Game game;
    const Core::GameSnapshot snapshot{game.snapshot()};
    const Core::Maze classic{Core::Maze::fromAscii(Core::c_classicLevel).value()};

    EXPECT_EQ(snapshot.mazeRows, classic.rows());
    EXPECT_EQ(snapshot.mazeCols, classic.cols());
    EXPECT_EQ(snapshot.tiles.size(), classic.tiles().size());
}

TEST(MazeAcceptance, SnapshotKeepsPreviousFields)
{
    Core::Game game{Tests::makePlayingGame()};
    const double before{game.totalTime()};
    game.update(0.5f);
    const Core::GameSnapshot snapshot{game.snapshot()};

    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_NEAR(snapshot.totalTime - before, 0.5, 1e-6);
}

TEST(MazeAcceptance, HelperCreatesGameOnGivenMap)
{
    const Core::Game game{Tests::makePlayingGame(Tests::c_smallLevel)};
    EXPECT_EQ(game.snapshot().mazeRows, 9);
}

TEST(MazeAcceptance, GameIsMovable)
{
    static_assert(std::is_move_constructible_v<Core::Game>);
    static_assert(std::is_move_assignable_v<Core::Game>);
}
