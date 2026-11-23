#pragma once

#include <cstdint>
#include <string_view>

namespace LifeExe::Core
{

/// High-level lifecycle state of the game.
enum class GameState : std::uint8_t
{
    Playing = 0,
    Paused,
    GameOver,
    Victory
};

/// Returns the string name of a GameState value.
constexpr std::string_view toString(GameState state)
{
    switch (state)
    {
        case GameState::Playing: return "Playing";
        case GameState::Paused: return "Paused";
        case GameState::GameOver: return "GameOver";
        case GameState::Victory: return "Victory";
    }
    return "Unknown";
}

/// Address of a maze cell: the row grows downwards, the column grows to the right.
struct GridPos
{
    int row{0};
    int col{0};

    friend constexpr bool operator==(const GridPos&, const GridPos&) = default;
};

constexpr GridPos operator+(GridPos lhs, GridPos rhs)
{
    return {lhs.row + rhs.row, lhs.col + rhs.col};
}

constexpr GridPos operator-(GridPos lhs, GridPos rhs)
{
    return {lhs.row - rhs.row, lhs.col - rhs.col};
}

constexpr GridPos operator*(int factor, GridPos pos)
{
    return {factor * pos.row, factor * pos.col};
}

/// Continuous position in tiles: x counts columns, y counts rows, origin at the top-left corner of the maze.
struct Vec2f
{
    float x{0.0f};
    float y{0.0f};
};

/// Content type of a single cell in the maze grid.
enum class TileType : std::uint8_t
{
    Empty = 0,
    Wall,
    Pellet,
    PowerPellet,
    GhostDoor,
    Tunnel
};

/// Entity category that determines cell walkability rules.
enum class Mover : std::uint8_t
{
    Player = 0,
    Ghost,
    GhostUsingDoor
};

/// Identifiers of the four ghosts.
enum class GhostId : std::uint8_t
{
    Ruby = 0,
    Rosie,
    Indigo,
    Coral
};

enum class Direction : std::uint8_t
{
    None = 0,
    Up,
    Down,
    Left,
    Right
};

constexpr Direction opposite(Direction direction)
{
    switch (direction)
    {
        case Direction::Up: return Direction::Down;
        case Direction::Down: return Direction::Up;
        case Direction::Left: return Direction::Right;
        case Direction::Right: return Direction::Left;
        case Direction::None: return Direction::None;
    }
    return Direction::None;
}

constexpr GridPos directionDelta(Direction direction)
{
    switch (direction)
    {
        case Direction::Up: return {-1, 0};
        case Direction::Down: return {1, 0};
        case Direction::Left: return {0, -1};
        case Direction::Right: return {0, 1};
        case Direction::None: return {0, 0};
    }
    return {0, 0};
}

}  // namespace LifeExe::Core
