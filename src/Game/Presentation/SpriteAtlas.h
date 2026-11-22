#pragma once

#include <algorithm>

#include "Types.h"

namespace LifeExe::Presentation
{

inline constexpr int c_tileSize{16};
inline constexpr int c_atlasCols{7};
inline constexpr int c_atlasRows{3};
inline constexpr int c_atlasWidth{c_atlasCols * c_tileSize};
inline constexpr int c_atlasHeight{c_atlasRows * c_tileSize};

struct SpriteRect
{
    int x{0};
    int y{0};
    int width{c_tileSize};
    int height{c_tileSize};

    friend constexpr bool operator==(const SpriteRect&, const SpriteRect&) = default;
};

constexpr SpriteRect tileRect(int col, int row)
{
    return {col * c_tileSize, row * c_tileSize, c_tileSize, c_tileSize};
}

constexpr SpriteRect playerMoveFrame(int frameIndex)
{
    const int wrapped{(frameIndex < 0 ? 0 : frameIndex) % 3};
    return tileRect(wrapped, 0);
}

constexpr SpriteRect playerDeathFrame(int frameIndex)
{
    const int clamped{std::clamp(frameIndex, 0, 3)};
    return tileRect(3 + clamped, 0);
}

constexpr SpriteRect ghostBodyFrame(int phaseIndex)
{
    const int wrapped{(phaseIndex < 0 ? 0 : phaseIndex) % 2};
    return tileRect(wrapped, 1);
}

constexpr SpriteRect ghostFrightenedFrame(bool flashing)
{
    return tileRect(flashing ? 3 : 2, 1);
}

constexpr SpriteRect ghostEyesFrame(Core::Direction direction)
{
    switch (direction)
    {
        case Core::Direction::Right: return tileRect(0, 2);
        case Core::Direction::Down: return tileRect(1, 2);
        case Core::Direction::Left: return tileRect(2, 2);
        case Core::Direction::Up: return tileRect(3, 2);
        case Core::Direction::None: return tileRect(2, 2);
    }
    return tileRect(2, 2);
}

constexpr SpriteRect pelletSprite()
{
    return tileRect(4, 2);
}

constexpr SpriteRect powerPelletSprite()
{
    return tileRect(5, 2);
}

constexpr SpriteRect lifeIconSprite()
{
    return tileRect(6, 2);
}

constexpr bool isInsideAtlas(SpriteRect rect)
{
    return rect.x >= 0 && rect.y >= 0 && rect.width == c_tileSize && rect.height == c_tileSize && (rect.x + rect.width) <= c_atlasWidth &&
           (rect.y + rect.height) <= c_atlasHeight;
}

static_assert(isInsideAtlas(playerMoveFrame(0)));
static_assert(isInsideAtlas(playerMoveFrame(2)));
static_assert(isInsideAtlas(playerDeathFrame(3)));
static_assert(isInsideAtlas(ghostBodyFrame(1)));
static_assert(isInsideAtlas(ghostFrightenedFrame(true)));
static_assert(isInsideAtlas(ghostEyesFrame(Core::Direction::Up)));
static_assert(isInsideAtlas(pelletSprite()));
static_assert(isInsideAtlas(powerPelletSprite()));
static_assert(isInsideAtlas(lifeIconSprite()));

}  // namespace LifeExe::Presentation
