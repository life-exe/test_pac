#pragma once

#include "GameSnapshot.h"
#include "Command.h"

#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

namespace LifeExe::Presentation
{

class SdlRenderer
{
public:
    SdlRenderer() = default;
    ~SdlRenderer();

    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;

    bool init(int width, int height, const std::string& title);
    void close();

    void render(const Core::GameSnapshot& snapshot) const;

    std::vector<Core::Command> processInput();
    [[nodiscard]] bool shouldClose() const;

private:
    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
    bool m_shouldClose{false};
};

}  // namespace LifeExe::Presentation
