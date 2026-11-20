#pragma once

#include <cstdint>
#include <string_view>

namespace LifeExe::Core
{

enum class GameState : std::uint8_t
{
    Playing = 0,
    Paused,
    GameOver
};

constexpr std::string_view toString(GameState state)
{
    switch (state)
    {
        case GameState::Playing: return "Playing";
        case GameState::Paused: return "Paused";
        case GameState::GameOver: return "GameOver";
    }
    return "Unknown";
}

// Address of a maze cell: the row grows downwards, the column grows to the right
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

// Continuous position in tiles: x counts columns, y counts rows, the origin is the top-left corner of the maze
struct Vec2f
{
    float x{0.0f};
    float y{0.0f};
};

}  // namespace LifeExe::Core