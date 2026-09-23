#pragma once

#include <iostream>
#include "Game.h"

namespace Game::Presentation
{

class ConsoleRenderer
{
public:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void render(const Core::Game& game) const;
};

}  // namespace Game::Presentation