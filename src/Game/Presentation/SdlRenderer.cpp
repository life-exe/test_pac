#include "SdlRenderer.h"

#include <SDL3/SDL.h>

#include <print>

namespace LifeExe::Presentation
{

SdlRenderer::~SdlRenderer()
{
    close();
}

bool SdlRenderer::init(int width, int height, const std::string& title)
{
    if (m_window != nullptr || m_renderer != nullptr)
    {
        return true;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::println(stderr, "SDL_Init Error: {}", SDL_GetError());
        return false;
    }

    m_window = SDL_CreateWindow(title.c_str(), width, height, 0);
    if (m_window == nullptr)
    {
        std::println(stderr, "SDL_CreateWindow Error: {}", SDL_GetError());
        close();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (m_renderer == nullptr)
    {
        std::println(stderr, "SDL_CreateRenderer Error: {}", SDL_GetError());
        close();
        return false;
    }

    SDL_SetRenderVSync(m_renderer, 1);
    m_shouldClose = false;
    return true;
}

void SdlRenderer::close()
{
    if (m_renderer != nullptr)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }

    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

bool SdlRenderer::shouldClose() const
{
    return m_shouldClose;
}

std::vector<Core::Command> SdlRenderer::processInput()
{
    std::vector<Core::Command> commands;

    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            m_shouldClose = true;
            commands.push_back(Core::Command::Quit);
        }
        else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
        {
            switch (event.key.key)
            {
                case SDLK_P:
                case SDLK_SPACE: commands.push_back(Core::Command::TogglePause); break;

                case SDLK_R: commands.push_back(Core::Command::Reset); break;

                case SDLK_ESCAPE:
                    m_shouldClose = true;
                    commands.push_back(Core::Command::Quit);
                    break;

                default: break;
            }
        }
    }

    return commands;
}

void SdlRenderer::render(const Core::GameSnapshot& snapshot) const
{
    if (m_renderer == nullptr)
    {
        return;
    }

    // 1. Clear background
    SDL_SetRenderDrawColor(m_renderer, 64, 16, 24, 255);
    SDL_RenderClear(m_renderer);

    // 2. Query window size
    int screenWidth{0};
    int screenHeight{0};
    SDL_GetWindowSize(m_window, &screenWidth, &screenHeight);

    const float centerX{static_cast<float>(screenWidth) / 2.0f};
    const float centerY{static_cast<float>(screenHeight) / 2.0f};

    // 3. Draw state indicator
    switch (snapshot.state)
    {
        case Core::GameState::Playing:
            SDL_SetRenderDrawColor(m_renderer, 46, 204, 113, 255);  // Green
            break;
        case Core::GameState::Paused:
            SDL_SetRenderDrawColor(m_renderer, 243, 156, 18, 255);  // Orange
            break;
        case Core::GameState::GameOver:
            SDL_SetRenderDrawColor(m_renderer, 231, 76, 60, 255);  // Red
            break;
    }

    const SDL_FRect centerRect{.x = centerX - 50.0f, .y = centerY - 50.0f, .w = 100.0f, .h = 100.0f};
    SDL_RenderFillRect(m_renderer, &centerRect);

    // 4. Present frame
    SDL_RenderPresent(m_renderer);
}

}  // namespace LifeExe::Presentation
