#include "HighScoreStorage.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>

#include <nlohmann/json.hpp>

namespace LifeExe
{

namespace
{

constexpr const char* c_highScoreKey{"highScore"};
constexpr int c_maxHighScore{std::numeric_limits<int>::max()};

// The file is user-editable, so a value outside the int range is clamped instead of wrapping around
int toHighScore(const nlohmann::json& value)
{
    if (value.is_number_unsigned())
    {
        const auto unsignedValue = value.get<std::uint64_t>();
        return static_cast<int>(std::min<std::uint64_t>(unsignedValue, c_maxHighScore));
    }
    const auto signedValue = value.get<std::int64_t>();
    return static_cast<int>(std::clamp<std::int64_t>(signedValue, 0, c_maxHighScore));
}

}  // namespace

int loadHighScore(const std::filesystem::path& path)
{
    std::ifstream file{path};
    if (!file.is_open())
    {
        return 0;
    }

    // allow_exceptions = false turns an empty or broken file into a discarded value instead of an exception
    const auto document = nlohmann::json::parse(file, nullptr, false);
    if (document.is_discarded() || !document.is_object())
    {
        return 0;
    }

    const auto field = document.find(c_highScoreKey);
    if (field == document.end() || !field->is_number_integer())
    {
        return 0;
    }
    return toHighScore(*field);
}

void saveHighScore(const std::filesystem::path& path, int highScore)
{
    std::ofstream file{path, std::ios::trunc};
    if (!file.is_open())
    {
        return;
    }

    // An explicit object avoids the initializer_list constructor, which could build an array instead
    nlohmann::json document = nlohmann::json::object();
    document[c_highScoreKey] = std::max(0, highScore);
    file << document.dump() << '\n';
}

}  // namespace LifeExe
