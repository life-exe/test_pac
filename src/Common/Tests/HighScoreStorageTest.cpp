#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "HighScoreStorage.h"

using namespace LifeExe;

namespace
{

class HighScoreStorageTest : public testing::Test
{
protected:
    // CTest may run the tests in parallel, so every test gets its own file named after it
    void SetUp() override
    {
        const std::string testName{testing::UnitTest::GetInstance()->current_test_info()->name()};
        m_path = std::filesystem::temp_directory_path() / ("maze_highscore_" + testName + ".json");
        removeFile();
    }

    void TearDown() override { removeFile(); }

    void writeFile(std::string_view content) const
    {
        std::ofstream file{m_path, std::ios::trunc};
        file << content;
    }

    const std::filesystem::path& path() const { return m_path; }

private:
    // The error code overload never throws, so cleanup cannot fail a test that already passed
    void removeFile() const
    {
        std::error_code error;
        std::filesystem::remove(m_path, error);
    }

    std::filesystem::path m_path;
};

}  // namespace

TEST_F(HighScoreStorageTest, MissingFileReturnsZero)
{
    ASSERT_FALSE(std::filesystem::exists(path()));
    EXPECT_EQ(loadHighScore(path()), 0);
}

TEST_F(HighScoreStorageTest, SaveThenLoadRoundTrips)
{
    saveHighScore(path(), 1234);
    EXPECT_EQ(loadHighScore(path()), 1234);

    // A later save replaces the record instead of appending to the file
    saveHighScore(path(), 50);
    EXPECT_EQ(loadHighScore(path()), 50);
}

TEST_F(HighScoreStorageTest, SaveWritesHighScoreObject)
{
    saveHighScore(path(), 1234);

    std::ifstream file{path()};
    const auto document = nlohmann::json::parse(file);
    const auto expected = nlohmann::json::parse(R"({"highScore": 1234})");
    EXPECT_EQ(document, expected);
}

TEST_F(HighScoreStorageTest, SaveClampsNegativeToZero)
{
    saveHighScore(path(), -5);

    std::ifstream file{path()};
    const auto document = nlohmann::json::parse(file);
    EXPECT_EQ(document.at("highScore").get<int>(), 0);
    EXPECT_EQ(loadHighScore(path()), 0);
}

TEST_F(HighScoreStorageTest, EmptyFileReturnsZero)
{
    writeFile("");
    EXPECT_EQ(loadHighScore(path()), 0);
}

TEST_F(HighScoreStorageTest, CorruptedJsonReturnsZero)
{
    writeFile(R"({"highScore": 12)");
    EXPECT_EQ(loadHighScore(path()), 0);
}

TEST_F(HighScoreStorageTest, WrongFieldTypeReturnsZero)
{
    writeFile(R"({"highScore": "100"})");
    EXPECT_EQ(loadHighScore(path()), 0);
}

TEST_F(HighScoreStorageTest, NegativeValueReturnsZero)
{
    writeFile(R"({"highScore": -7})");
    EXPECT_EQ(loadHighScore(path()), 0);
}
