#pragma once

#include <istream>
#include <string>

/// Game logic: everything the executable and the tests share.
namespace LifeExe
{

/// Game settings read from config.json at startup.
struct GameConfig
{
    std::string title;  ///< Window title.
    int width{0};       ///< Window width in pixels.
    int height{0};      ///< Window height in pixels.
};

GameConfig parseConfig(std::istream& input);

}  // namespace LifeExe
