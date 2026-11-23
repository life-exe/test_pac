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

class Maze
{
public:
    static std::expected<Maze, MazeError> fromAscii(std::string_view asciiMap);

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    std::span<const TileType> tiles() const { return m_tiles; }  // row-major: row * cols() + col

    TileType tileAt(GridPos pos) const;  // returns Wall for out-of-bounds
    bool isWalkable(GridPos pos, Mover mover) const;

    GridPos playerSpawn() const { return m_playerSpawn; }
    GridPos ghostSpawn(GhostId id) const;
    GridPos tunnelExit(GridPos pos) const;                 // opposite edge in the same row, or pos if not a tunnel
    GridPos houseExitTile() const { return m_houseExit; }  // cell directly above the ghost door
    GridPos scatterCorner(GhostId id) const;

    int remainingPellets() const { return m_remainingPellets; }
    TileType consumeTileAt(GridPos pos);  // replaces pellet with Empty, returns previous tile

private:
    Maze() = default;

    int m_rows{0};
    int m_cols{0};
    int m_remainingPellets{0};
    std::vector<TileType> m_tiles;
    GridPos m_playerSpawn;
    std::array<GridPos, 4> m_ghostSpawns{};
    GridPos m_houseExit;
};

}  // namespace LifeExe::Core
