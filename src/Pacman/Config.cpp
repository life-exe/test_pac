#include "Config.h"

#include <format>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>

namespace LifeExe
{

namespace
{
const nlohmann::json& require(const nlohmann::json& object, std::string_view key)
{
    if (!object.contains(key))
    {
        throw std::runtime_error(std::format("config: key '{}' is missing", key));
    }
    return object.at(key);
}
}  // namespace

GameConfig parseConfig(std::istream& input)
{
    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(input);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        throw std::runtime_error(std::format("config: invalid JSON: {}", e.what()));
    }

    try
    {
        GameConfig config;
        config.title = require(json, "title").get<std::string>();
        const auto& window = require(json, "window");
        config.width = require(window, "width").get<int>();
        config.height = require(window, "height").get<int>();
        return config;
    }
    catch (const nlohmann::json::type_error& e)
    {
        throw std::runtime_error(std::format("config: wrong value type: {}", e.what()));
    }
}

std::string spoiled(std::string value)
{
    std::string copy = "";
    for (int i = 0; i < value.size(); i++)
    {
        copy += value[i];
    }
    return copy;
}

}  // namespace LifeExe