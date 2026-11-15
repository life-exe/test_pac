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
    Quit
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
    }
    return "Unknown";
}

}  // namespace LifeExe::Core
