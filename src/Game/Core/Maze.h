#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <vector>

#include "Types.h"

namespace LifeExe::Core
{

/// Validation error returned by Maze::fromAscii when an ASCII level map is malformed.
enum class MazeError : std::uint8_t
{
    EmptyMap = 0,
    RaggedRows,
    UnknownSymbol,
    MissingPlayerSpawn,
    MissingGhostSpawn,
    DuplicateSpawn,
    MissingGhostDoor,
    UnpairedTunnel
};

/// Immutable tile grid parsed from an ASCII level map.
class Maze
{
public:
    /// Parses an ASCII map into a valid Maze or returns a MazeError.
    static std::expected<Maze, MazeError> fromAscii(std::string_view asciiMap);

    /// Number of rows in the maze grid.
    int rows() const { return m_rows; }

    /// Number of columns in the maze grid.
    int cols() const { return m_cols; }

    /// Flat row-major view of all cells (index = row * cols() + col).
    std::span<const TileType> tiles() const { return m_tiles; }

    /// Returns the tile at pos, or TileType::Wall for out-of-bounds coordinates.
    TileType tileAt(GridPos pos) const;

    /// Reports whether an entity of the given category may enter pos.
    bool isWalkable(GridPos pos, Mover mover) const;

    /// Starting cell of the player.
    GridPos playerSpawn() const { return m_playerSpawn; }

    /// Starting cell of the specified ghost.
    GridPos ghostSpawn(GhostId id) const;

    /// Paired tunnel exit on the opposite edge of the same row, or pos if not a tunnel.
    GridPos tunnelExit(GridPos pos) const;

    /// Cell directly above the first ghost house door.
    GridPos houseExitTile() const { return m_houseExit; }

    /// Off-grid corner target used by a ghost in Scatter mode.
    GridPos scatterCorner(GhostId id) const;

private:
    Maze() = default;

    int m_rows{0};
    int m_cols{0};
    std::vector<TileType> m_tiles;
    GridPos m_playerSpawn;
    std::array<GridPos, 4> m_ghostSpawns{};
    GridPos m_houseExit;
};

}  // namespace LifeExe::Core
