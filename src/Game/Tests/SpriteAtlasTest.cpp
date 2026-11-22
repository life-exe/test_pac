#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#include "SpriteAtlas.h"

using namespace LifeExe;
using Presentation::SpriteRect;

namespace
{

// Every frame the game may ask for: generate_assets.py must fill exactly these 18 tiles
constexpr std::array<SpriteRect, 18> c_allFrames{
    Presentation::playerMoveFrame(0),
    Presentation::playerMoveFrame(1),
    Presentation::playerMoveFrame(2),
    Presentation::playerDeathFrame(0),
    Presentation::playerDeathFrame(1),
    Presentation::playerDeathFrame(2),
    Presentation::playerDeathFrame(3),
    Presentation::ghostBodyFrame(0),
    Presentation::ghostBodyFrame(1),
    Presentation::ghostFrightenedFrame(false),
    Presentation::ghostFrightenedFrame(true),
    Presentation::ghostEyesFrame(Core::Direction::Right),
    Presentation::ghostEyesFrame(Core::Direction::Down),
    Presentation::ghostEyesFrame(Core::Direction::Left),
    Presentation::ghostEyesFrame(Core::Direction::Up),
    Presentation::pelletSprite(),
    Presentation::powerPelletSprite(),
    Presentation::lifeIconSprite(),
};

// Kept out of the loop: each GoogleTest macro adds branches, and the loop would exceed the complexity limit
void expectInsideAtlas(const SpriteRect& rect)
{
    EXPECT_TRUE(Presentation::isInsideAtlas(rect));
    EXPECT_GE(rect.x, 0);
    EXPECT_GE(rect.y, 0);
    EXPECT_LE(rect.x + rect.width, Presentation::c_atlasWidth);
    EXPECT_LE(rect.y + rect.height, Presentation::c_atlasHeight);
}

}  // namespace

TEST(SpriteAtlasTest, ConstantsDescribe112x48Atlas)
{
    EXPECT_EQ(Presentation::c_tileSize, 16);
    EXPECT_EQ(Presentation::c_atlasCols, 7);
    EXPECT_EQ(Presentation::c_atlasRows, 3);
    EXPECT_EQ(Presentation::c_atlasWidth, 112);
    EXPECT_EQ(Presentation::c_atlasHeight, 48);
}

TEST(SpriteAtlasTest, AllFramesFitInsideAtlas)
{
    for (std::size_t index{0}; index < c_allFrames.size(); ++index)
    {
        SCOPED_TRACE(testing::Message() << "frame " << index);
        expectInsideAtlas(c_allFrames.at(index));
    }
}

TEST(SpriteAtlasTest, AllFramesAreDistinct)
{
    for (std::size_t first{0}; first < c_allFrames.size(); ++first)
    {
        for (std::size_t second{first + 1}; second < c_allFrames.size(); ++second)
        {
            EXPECT_NE(c_allFrames.at(first), c_allFrames.at(second)) << "frames " << first << " and " << second;
        }
    }
}

TEST(SpriteAtlasTest, PlayerMoveFrameWrapsAround)
{
    EXPECT_EQ(Presentation::playerMoveFrame(3), Presentation::playerMoveFrame(0));
    EXPECT_EQ(Presentation::playerMoveFrame(5), Presentation::playerMoveFrame(2));
    EXPECT_EQ(Presentation::playerMoveFrame(-1), Presentation::playerMoveFrame(0));
    EXPECT_EQ(Presentation::playerMoveFrame(1), Presentation::tileRect(1, 0));
}

TEST(SpriteAtlasTest, GhostBodyFrameWrapsAround)
{
    EXPECT_EQ(Presentation::ghostBodyFrame(2), Presentation::ghostBodyFrame(0));
    EXPECT_EQ(Presentation::ghostBodyFrame(3), Presentation::ghostBodyFrame(1));
    EXPECT_EQ(Presentation::ghostBodyFrame(-1), Presentation::ghostBodyFrame(0));
    EXPECT_EQ(Presentation::ghostBodyFrame(1), Presentation::tileRect(1, 1));
}

TEST(SpriteAtlasTest, PlayerDeathFrameIsClamped)
{
    EXPECT_EQ(Presentation::playerDeathFrame(-1), Presentation::playerDeathFrame(0));
    EXPECT_EQ(Presentation::playerDeathFrame(4), Presentation::playerDeathFrame(3));
    EXPECT_EQ(Presentation::playerDeathFrame(100), Presentation::playerDeathFrame(3));
    EXPECT_EQ(Presentation::playerDeathFrame(0), Presentation::tileRect(3, 0));
    EXPECT_EQ(Presentation::playerDeathFrame(3), Presentation::tileRect(6, 0));
}

TEST(SpriteAtlasTest, GhostEyesFollowDirection)
{
    EXPECT_EQ(Presentation::ghostEyesFrame(Core::Direction::Up), Presentation::tileRect(3, 2));
    // A ghost without direction looks left, like the default facing
    EXPECT_EQ(Presentation::ghostEyesFrame(Core::Direction::None), Presentation::ghostEyesFrame(Core::Direction::Left));
}
