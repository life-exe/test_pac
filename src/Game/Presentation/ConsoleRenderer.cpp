#include "ConsoleRenderer.h"
#include <print>

namespace LifeExe::Presentation
{

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ConsoleRenderer::render(const Core::GameSnapshot& snapshot) const
{
    std::println("[Frame Render] State: {}, Time: {:.2f}s", Core::toString(snapshot.state), snapshot.totalTime);
}

}  // namespace LifeExe::Presentation
