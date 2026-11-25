#pragma once

#include <cstdint>
#include <string_view>

namespace LifeExe::Core
{

enum class Command : std::uint8_t
{
    None = 0,
    Pause,
    Resume,
    TogglePause,
    Reset,
    Quit,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Confirm
};

constexpr std::string_view toString(Command command)
{
    switch (command)
    {
        case Command::None: return "None";
        case Command::Pause: return "Pause";
        case Command::Resume: return "Resume";
        case Command::TogglePause: return "TogglePause";
        case Command::Reset: return "Reset";
        case Command::Quit: return "Quit";
        case Command::MoveUp: return "MoveUp";
        case Command::MoveDown: return "MoveDown";
        case Command::MoveLeft: return "MoveLeft";
        case Command::MoveRight: return "MoveRight";
        case Command::Confirm: return "Confirm";
    }
    return "Unknown";
}

}  // namespace LifeExe::Core
