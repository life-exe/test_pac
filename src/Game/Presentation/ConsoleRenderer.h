#pragma once

#include "Game.h"

namespace LifeExe::Presentation
{

class ConsoleRenderer
{
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void render(const Core::Game& game) const;
};

}  // namespace LifeExe::Presentation