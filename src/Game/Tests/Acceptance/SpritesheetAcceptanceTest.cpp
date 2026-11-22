#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "SpriteAtlas.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::Direction;
using Presentation::SpriteRect;

namespace
{

constexpr float c_tolerance{1e-3f};

constexpr float secondsFor(float tiles)
{
    return tiles / Core::c_playerSpeed;
}

bool fitsAtlas(SpriteRect rect)
{
    return rect.x >= 0 && rect.y >= 0 && rect.width == Presentation::c_tileSize && rect.height == Presentation::c_tileSize &&
           (rect.x + rect.width) <= Presentation::c_atlasWidth && (rect.y + rect.height) <= Presentation::c_atlasHeight;
}

}  // namespace

TEST(SpritesheetAcceptance, AnimTimeStartsAtZeroAndGrowsWhileMoving)
{
    Core::Game game{Tests::makePlayingGame()};
    EXPECT_FLOAT_EQ(game.snapshot().player.animTime, 0.0f);

    game.handleCommand(Command::MoveRight);
    const float step{secondsFor(2.0f)};
    game.update(step);

    EXPECT_TRUE(game.snapshot().player.isMoving);
    EXPECT_NEAR(game.snapshot().player.animTime, step, c_tolerance);
}

TEST(SpritesheetAcceptance, AnimTimeFreezesWhenStoppedBeforeAWall)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveDown);
    game.update(secondsFor(4.0f));
    EXPECT_FALSE(game.snapshot().player.isMoving);

    const float stoppedAnimTime{game.snapshot().player.animTime};
    game.update(1.0f);

    EXPECT_FLOAT_EQ(game.snapshot().player.animTime, stoppedAnimTime);
}

TEST(SpritesheetAcceptance, AnimTimeFreezesOnPauseAndResetsOnReset)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Command::MoveRight);
    game.update(secondsFor(1.0f));
    const float movedAnimTime{game.snapshot().player.animTime};
    EXPECT_GT(movedAnimTime, 0.0f);

    game.handleCommand(Command::Pause);
    game.update(1.0f);
    EXPECT_FLOAT_EQ(game.snapshot().player.animTime, movedAnimTime);

    game.handleCommand(Command::Reset);
    EXPECT_FLOAT_EQ(game.snapshot().player.animTime, 0.0f);
}

TEST(SpritesheetAcceptance, SpriteAtlasDimensionsAndTileSizeMatchContract)
{
    EXPECT_EQ(Presentation::c_tileSize, 16);
    EXPECT_EQ(Presentation::c_atlasCols, 7);
    EXPECT_EQ(Presentation::c_atlasRows, 3);
    EXPECT_EQ(Presentation::c_atlasWidth, 112);
    EXPECT_EQ(Presentation::c_atlasHeight, 48);
}

TEST(SpritesheetAcceptance, AllAtlasFramesAreDistinctAndInsideBounds)
{
    const std::array<SpriteRect, 18> frames{
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
        Presentation::ghostEyesFrame(Direction::Right),
        Presentation::ghostEyesFrame(Direction::Down),
        Presentation::ghostEyesFrame(Direction::Left),
        Presentation::ghostEyesFrame(Direction::Up),
        Presentation::pelletSprite(),
        Presentation::powerPelletSprite(),
        Presentation::lifeIconSprite(),
    };

    for (std::size_t i{0}; i < frames.size(); ++i)
    {
        EXPECT_TRUE(fitsAtlas(frames[i])) << "Frame index " << i;
        for (std::size_t j{i + 1}; j < frames.size(); ++j)
        {
            EXPECT_NE(frames[i], frames[j]) << "Duplicate frame at " << i << " and " << j;
        }
    }
}

TEST(SpritesheetAcceptance, LoopingAndClampingFrameIndicesStayValid)
{
    EXPECT_EQ(Presentation::playerMoveFrame(3), Presentation::playerMoveFrame(0));
    EXPECT_EQ(Presentation::playerMoveFrame(4), Presentation::playerMoveFrame(1));
    EXPECT_EQ(Presentation::playerDeathFrame(10), Presentation::playerDeathFrame(3));
    EXPECT_EQ(Presentation::playerDeathFrame(-1), Presentation::playerDeathFrame(0));
    EXPECT_EQ(Presentation::ghostBodyFrame(2), Presentation::ghostBodyFrame(0));
    EXPECT_TRUE(fitsAtlas(Presentation::ghostEyesFrame(Direction::None)));
}
