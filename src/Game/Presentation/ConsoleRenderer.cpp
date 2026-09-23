#include "ConsoleRenderer.h"
#include <print>

namespace Game::Presentation
{

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ConsoleRenderer::render(const Core::Game& game) const
{
    std::println("[Frame Render] State: {}, Time: {:.2f}s", Core::toString(game.state()), game.totalTime());
}

}  // namespace Game::Presentation