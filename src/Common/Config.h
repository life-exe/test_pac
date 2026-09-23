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

/// Reads the game configuration from a JSON stream.
///
/// Expected document: `{"title": "...", "window": {"width": N, "height": N}}`.
///
/// @param input stream with the JSON document, for example an open config.json.
/// @return the parsed configuration.
/// @throws std::runtime_error if the document is not valid JSON or a key is missing;
///         the message names the key.
GameConfig parseConfig(std::istream& input);

}  // namespace LifeExe
