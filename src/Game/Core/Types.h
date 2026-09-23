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

}  // namespace LifeExe::Core