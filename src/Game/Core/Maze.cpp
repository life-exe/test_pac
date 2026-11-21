#include "Maze.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>

namespace LifeExe::Core
{

namespace
{

struct ParseState
{
    std::optional<GridPos> player;
    std::array<std::optional<GridPos>, 4> ghosts;
    std::optional<GridPos> firstDoor;
    std::vector<std::vector<int>> tunnelCols;
};

std::size_t tileIndex(int row, int col, int cols)
{
    return (static_cast<std::size_t>(row) * static_cast<std::size_t>(cols)) + static_cast<std::size_t>(col);
}

std::vector<std::string_view> splitLines(std::string_view text)
{
    std::vector<std::string_view> lines;
    std::size_t begin{0};
    while (begin <= text.size())
    {
        const std::size_t end{text.find('\n', begin)};
        if (end == std::string_view::npos)
        {
            lines.push_back(text.substr(begin));
            break;
        }
        lines.push_back(text.substr(begin, end - begin));
        begin = end + 1;
    }
    return lines;
}

std::expected<TileType, MazeError> parseCell(char symbol, GridPos pos, ParseState& state)
{
    switch (symbol)
    {
        case '#': return TileType::Wall;
        case '.': return TileType::Pellet;
        case 'o': return TileType::PowerPellet;
        case ' ': return TileType::Empty;
        case '=':
            if (!state.firstDoor)
            {
                state.firstDoor = pos;
            }
            return TileType::GhostDoor;
        case 'T': state.tunnelCols[static_cast<std::size_t>(pos.row)].push_back(pos.col); return TileType::Tunnel;
        case 'P':
            if (state.player)
            {
                return std::unexpected{MazeError::DuplicateSpawn};
            }
            state.player = pos;
            return TileType::Empty;
        case '1':
        case '2':
        case '3':
        case '4':
        {
            const auto index{static_cast<std::size_t>(symbol - '1')};
            if (state.ghosts[index])
            {
                return std::unexpected{MazeError::DuplicateSpawn};
            }
            state.ghosts[index] = pos;
            return TileType::Empty;
        }
        default: return std::unexpected{MazeError::UnknownSymbol};
    }
}

bool hasUnpairedTunnel(const std::vector<std::vector<int>>& tunnelCols, int cols)
{
    return std::ranges::any_of(tunnelCols,
        [cols](const std::vector<int>& rowTunnels)
        {
            const bool paired{rowTunnels.size() == 2 && rowTunnels.front() == 0 && rowTunnels.back() == cols - 1};
            return !rowTunnels.empty() && !paired;
        });
}

}  // namespace

std::expected<Maze, MazeError> Maze::fromAscii(std::string_view asciiMap)
{
    // One line terminator at the very end is allowed, so adjacent string literals work naturally
    if (asciiMap.ends_with('\n'))
    {
        asciiMap.remove_suffix(1);
    }
    if (asciiMap.empty())
    {
        return std::unexpected{MazeError::EmptyMap};
    }

    const std::vector<std::string_view> lines{splitLines(asciiMap)};
    const std::size_t width{lines.front().size()};
    for (const std::string_view line : lines)
    {
        if (line.size() != width)
        {
            return std::unexpected{MazeError::RaggedRows};
        }
    }

    Maze maze;
    maze.m_rows = static_cast<int>(lines.size());
    maze.m_cols = static_cast<int>(width);
    maze.m_tiles.assign(lines.size() * width, TileType::Empty);

    ParseState state;
    state.tunnelCols.resize(lines.size());

    for (int row{0}; row < maze.m_rows; ++row)
    {
        for (int col{0}; col < maze.m_cols; ++col)
        {
            const char symbol{lines[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)]};
            const std::expected<TileType, MazeError> tile{parseCell(symbol, {row, col}, state)};
            if (!tile)
            {
                return std::unexpected{tile.error()};
            }
            maze.m_tiles[tileIndex(row, col, maze.m_cols)] = *tile;
        }
    }

    if (!state.player)
    {
        return std::unexpected{MazeError::MissingPlayerSpawn};
    }
    for (std::size_t index{0}; index < state.ghosts.size(); ++index)
    {
        // Bind once: the optional-access analysis cannot prove two operator[] calls hit the same element
        const std::optional<GridPos>& ghost{state.ghosts[index]};
        if (!ghost)
        {
            return std::unexpected{MazeError::MissingGhostSpawn};
        }
        maze.m_ghostSpawns[index] = *ghost;
    }
    if (!state.firstDoor)
    {
        return std::unexpected{MazeError::MissingGhostDoor};
    }
    if (hasUnpairedTunnel(state.tunnelCols, maze.m_cols))
    {
        return std::unexpected{MazeError::UnpairedTunnel};
    }

    maze.m_playerSpawn = *state.player;
    maze.m_houseExit = {state.firstDoor->row - 1, state.firstDoor->col};
    return maze;
}

TileType Maze::tileAt(GridPos pos) const
{
    if (pos.row < 0 || pos.row >= m_rows || pos.col < 0 || pos.col >= m_cols)
    {
        return TileType::Wall;
    }
    return m_tiles[tileIndex(pos.row, pos.col, m_cols)];
}

bool Maze::isWalkable(GridPos pos, Mover mover) const
{
    switch (tileAt(pos))
    {
        case TileType::Wall: return false;
        case TileType::GhostDoor: return mover == Mover::GhostUsingDoor;
        case TileType::Empty:
        case TileType::Pellet:
        case TileType::PowerPellet:
        case TileType::Tunnel: return true;
    }
    return false;
}

GridPos Maze::ghostSpawn(GhostId id) const
{
    return m_ghostSpawns[static_cast<std::size_t>(id)];
}

GridPos Maze::tunnelExit(GridPos pos) const
{
    if (tileAt(pos) != TileType::Tunnel)
    {
        return pos;
    }
    return {pos.row, pos.col == 0 ? m_cols - 1 : 0};
}

GridPos Maze::scatterCorner(GhostId id) const
{
    switch (id)
    {
        case GhostId::Ruby: return {-3, m_cols - 3};
        case GhostId::Rosie: return {-3, 2};
        case GhostId::Indigo: return {m_rows + 1, m_cols - 1};
        case GhostId::Coral: return {m_rows + 1, 0};
    }
    return {};
}

}  // namespace LifeExe::Core
