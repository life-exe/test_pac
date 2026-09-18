#include <sstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "Config.h"

using namespace LifeExe;

TEST(ConfigTest, ShouldReadTitleAndWindow)
{
    std::istringstream input{R"({"title": "Pac-Man", "window": {"width": 800, "height": 600}})"};

    const GameConfig config = parseConfig(input);

    EXPECT_EQ(config.title, "Pac-Man");
    EXPECT_EQ(config.width, 800);
    EXPECT_EQ(config.height, 600);
}

TEST(ConfigTest, ShouldNameMissingKey)
{
    std::istringstream input{R"({"title": "Pac-Man", "window": {"height": 600}})"};

    try
    {
        parseConfig(input);
        FAIL() << "expected std::runtime_error";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_NE(std::string{e.what()}.find("width"), std::string::npos) << e.what();
    }
}

TEST(ConfigTest, ShouldRejectWrongValueType)
{
    std::istringstream input{R"({"title": "Pac-Man", "window 600}})"};

    EXPECT_THROW(parseConfig(input), std::runtime_error);
}
