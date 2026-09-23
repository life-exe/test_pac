#pragma once

#include "Game.h"
#include <string>

struct SDL_Window;
struct SDL_Renderer;

namespace LifeExe::Presentation
{

class SdlRenderer
{
public:
    SdlRenderer() = default;
    ~SdlRenderer();

    // Prevent accidental copying of window handle / graphics context
    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;

    bool init(int width, int height, const std::string& title);
    void processInput(Core::Game& game);
    void render(const Core::Game& game) const;
    void close();
    [[nodiscard]] bool shouldClose() const;

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    bool m_shouldClose{false};
};

}  // namespace LifeExe::Presentation