#pragma once

#include <cstdint>
#include <string_view>

namespace LifeExe::Core
{

enum class GameEvent : std::uint8_t
{
    PelletEaten = 0,
    PowerPelletEaten,
    GhostEaten,
    PlayerLostLife,
    RoundStarted,
    Victory,
    GameOver
};

constexpr std::string_view toString(GameEvent event)
{
    switch (event)
    {
        case GameEvent::PelletEaten: return "PelletEaten";
        case GameEvent::PowerPelletEaten: return "PowerPelletEaten";
        case GameEvent::GhostEaten: return "GhostEaten";
        case GameEvent::PlayerLostLife: return "PlayerLostLife";
        case GameEvent::RoundStarted: return "RoundStarted";
        case GameEvent::Victory: return "Victory";
        case GameEvent::GameOver: return "GameOver";
    }
    return "Unknown";
}

}  // namespace LifeExe::Core
