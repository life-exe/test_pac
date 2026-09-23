#include "RaylibRenderer.h"

#include <raylib.h>
#include <format>

namespace LifeExe::Presentation
{

RaylibRenderer::~RaylibRenderer()
{
    close();
}

void RaylibRenderer::init(int width, int height, const std::string& title)
{
    if (!m_initialized)
    {
        InitWindow(width, height, title.c_str());
        SetTargetFPS(60);
        m_initialized = true;
    }
}

void RaylibRenderer::render(const Core::Game& game) const
{
    if (!m_initialized)
    {
        return;
    }

    BeginDrawing();
    ClearBackground(Color{.r = 24, .g = 24, .b = 32, .a = 255});

    // 1. Draw HUD: FPS and Total Time
    DrawFPS(10, 10);
    const std::string timeText{std::format("Simulation Time: {:.2f}s", game.totalTime())};
    DrawText(timeText.c_str(), 10, 35, 20, LIGHTGRAY);

    // 2. Draw Game State Badge
    const auto [stateStr, stateColor] = [&]() -> std::pair<const char*, Color>
    {
        switch (game.state())
        {
            case Core::GameState::Playing: return {"STATE: PLAYING", GREEN};
            case Core::GameState::Paused: return {"STATE: PAUSED", ORANGE};
            case Core::GameState::GameOver: return {"STATE: GAME OVER", RED};
            default: return {"UNKNOWN", WHITE};
        }
    }();

    DrawText(stateStr, 10, 65, 20, stateColor);

    // 3. Draw visual primitive driven by simulation state
    const int screenWidth{GetScreenWidth()};
    const int screenHeight{GetScreenHeight()};
    const int centerX{screenWidth / 2};
    const int centerY{screenHeight / 2};

    if (game.state() == Core::GameState::Playing)
    {
        DrawCircle(centerX, centerY, 50.0f, SKYBLUE);
    }
    else if (game.state() == Core::GameState::Paused)
    {
        DrawRectangle(centerX - 40, centerY - 40, 80, 80, ORANGE);
        DrawText("PRESS [P] TO RESUME", centerX - 110, centerY + 60, 20, RAYWHITE);
    }
    else
    {
        DrawText("GAME OVER - PRESS [R] TO RESTART", centerX - 180, centerY, 20, RED);
    }

    // Controls tip at the bottom
    DrawText("Controls: [P] Pause/Resume | [R] Reset | [ESC] Quit", 10, screenHeight - 30, 18, GRAY);

    EndDrawing();
}

void RaylibRenderer::processInput(Core::Game& game) const
{
    if (!m_initialized)
    {
        return;
    }

    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
    {
        if (game.state() == Core::GameState::Playing)
        {
            game.handleCommand(Core::Command::Pause);
        }
        else if (game.state() == Core::GameState::Paused)
        {
            game.handleCommand(Core::Command::Resume);
        }
    }

    if (IsKeyPressed(KEY_R))
    {
        game.handleCommand(Core::Command::Reset);
    }

    if (IsKeyPressed(KEY_ESCAPE) || WindowShouldClose())
    {
        game.handleCommand(Core::Command::Quit);
    }
}

void RaylibRenderer::close()
{
    if (m_initialized)
    {
        CloseWindow();
        m_initialized = false;
    }
}

bool RaylibRenderer::shouldClose() const
{
    return !m_initialized || WindowShouldClose();
}

}  // namespace LifeExe::Presentation