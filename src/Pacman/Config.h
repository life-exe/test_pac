#pragma once

#include <istream>
#include <string>

namespace LifeExe
{

struct GameConfig
{
    std::string title;
    int width{0};
    int height{0};
};

// Reads the game configuration from a JSON stream.
// Throws std::runtime_error with the missing or invalid key name.
GameConfig parseConfig(std::istream& input);

}  // namespace LifeExe