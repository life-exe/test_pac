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
        SetConfigFlags(FLAG_MSAA_4X_HINT);
        InitWindow(width, height, title.c_str());
        SetTargetFPS(60);
        // Esc is handled as a regular key and reaches the core as Command::Quit
        SetExitKey(KEY_NULL);
        m_initialized = true;
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

void RaylibRenderer::render(const Core::GameSnapshot& snapshot) const
{
    if (!m_initialized)
    {
        return;
    }

    BeginDrawing();
    ClearBackground(DARKGRAY);

    // 1. Draw HUD: FPS and Total Time
    DrawFPS(10, 10);
    const std::string timeText{std::format("Simulation Time: {:.2f}s", snapshot.totalTime)};
    DrawText(timeText.c_str(), 10, 35, 20, LIGHTGRAY);

    // 2. Draw Game State Badge
    const char* stateStr{"UNKNOWN"};
    Color stateColor{WHITE};

    switch (snapshot.state)
    {
        case Core::GameState::Playing:
            stateStr = "STATE: PLAYING";
            stateColor = GREEN;
            break;
        case Core::GameState::Paused:
            stateStr = "STATE: PAUSED";
            stateColor = ORANGE;
            break;
        case Core::GameState::GameOver:
            stateStr = "STATE: GAME OVER";
            stateColor = RED;
            break;
    }

    DrawText(stateStr, 10, 65, 20, stateColor);

    // 3. Draw visual primitive driven by simulation state
    const int screenWidth{GetScreenWidth()};
    const int screenHeight{GetScreenHeight()};
    const int centerX{screenWidth / 2};
    const int centerY{screenHeight / 2};

    if (snapshot.state == Core::GameState::Playing)
    {
        DrawCircle(centerX, centerY, 50.0f, SKYBLUE);
    }
    else if (snapshot.state == Core::GameState::Paused)
    {
        DrawRectangle(centerX - 40, centerY - 40, 80, 80, ORANGE);
        DrawText("PRESS [P] TO RESUME", centerX - 110, centerY + 60, 20, RAYWHITE);
    }
    else
    {
        DrawText("GAME OVER", centerX - 55, centerY, 20, RED);
    }

    // Controls tip at the bottom
    DrawText("Controls: [P] Pause/Resume | [R] Reset | [ESC] Quit", 10, screenHeight - 30, 18, GRAY);

    EndDrawing();
}

std::vector<Core::Command> RaylibRenderer::processInput() const
{
    std::vector<Core::Command> commands;
    if (!m_initialized)
    {
        return commands;
    }

    // The core decides whether this means pause or resume
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
    {
        commands.push_back(Core::Command::TogglePause);
    }

    if (IsKeyPressed(KEY_R))
    {
        commands.push_back(Core::Command::Reset);
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        commands.push_back(Core::Command::Quit);
    }

    return commands;
}

}  // namespace LifeExe::Presentation
