#include "RaylibRenderer.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <string>

namespace LifeExe::Presentation
{

namespace
{

// Bands above and below the maze are reserved for the HUD, so text never covers the playfield
constexpr int c_hudMargin{48};
constexpr int c_hudFontSize{20};

constexpr Color c_wallColor{33, 33, 255, 255};
constexpr Color c_ghostDoorColor{255, 184, 222, 255};

struct KeyBinding
{
    KeyboardKey key{KEY_NULL};
    Core::Command command{Core::Command::None};
};

// WASD and the arrows are equal alternatives, so both map to the same movement commands
constexpr std::array c_moveBindings{
    KeyBinding{.key = KEY_W, .command = Core::Command::MoveUp},
    KeyBinding{.key = KEY_UP, .command = Core::Command::MoveUp},
    KeyBinding{.key = KEY_S, .command = Core::Command::MoveDown},
    KeyBinding{.key = KEY_DOWN, .command = Core::Command::MoveDown},
    KeyBinding{.key = KEY_A, .command = Core::Command::MoveLeft},
    KeyBinding{.key = KEY_LEFT, .command = Core::Command::MoveLeft},
    KeyBinding{.key = KEY_D, .command = Core::Command::MoveRight},
    KeyBinding{.key = KEY_RIGHT, .command = Core::Command::MoveRight},
};

// Width and height travel together, so they cannot be swapped at a call site
struct ScreenSize
{
    int width{0};
    int height{0};
};

struct MazeLayout
{
    int tileSize{1};
    int originX{0};
    int originY{0};
};

// An integer tile size keeps every wall on whole pixels, so the grid does not shimmer
MazeLayout computeLayout(const Core::GameSnapshot& snapshot, ScreenSize screen)
{
    const int availableHeight{screen.height - (2 * c_hudMargin)};
    const int tileSize{std::max(1, std::min(screen.width / snapshot.mazeCols, availableHeight / snapshot.mazeRows))};
    return {.tileSize = tileSize,
        .originX = (screen.width - (snapshot.mazeCols * tileSize)) / 2,
        .originY = c_hudMargin + ((availableHeight - (snapshot.mazeRows * tileSize)) / 2)};
}

void drawWall(Rectangle tile)
{
    const float inset{tile.width * 0.1f};
    const Rectangle body{tile.x + inset, tile.y + inset, tile.width - (2.0f * inset), tile.height - (2.0f * inset)};
    DrawRectangleRounded(body, 0.4f, 6, c_wallColor);
}

void drawGhostDoor(Rectangle tile)
{
    const float barHeight{std::max(1.0f, tile.height / 4.0f)};
    const Rectangle bar{tile.x, tile.y + ((tile.height - barHeight) / 2.0f), tile.width, barHeight};
    DrawRectangleRec(bar, c_ghostDoorColor);
}

void drawMaze(const Core::GameSnapshot& snapshot, const MazeLayout& layout)
{
    const auto size{static_cast<float>(layout.tileSize)};
    for (int row{0}; row < snapshot.mazeRows; ++row)
    {
        for (int col{0}; col < snapshot.mazeCols; ++col)
        {
            const Rectangle tile{static_cast<float>(layout.originX + (col * layout.tileSize)),
                static_cast<float>(layout.originY + (row * layout.tileSize)), size, size};
            const std::size_t index{
                (static_cast<std::size_t>(row) * static_cast<std::size_t>(snapshot.mazeCols)) + static_cast<std::size_t>(col)};
            switch (snapshot.tiles[index])
            {
                case Core::TileType::Wall: drawWall(tile); break;
                case Core::TileType::GhostDoor: drawGhostDoor(tile); break;
                // Pellets and the rest get their look in later tasks
                case Core::TileType::Empty:
                case Core::TileType::Pellet:
                case Core::TileType::PowerPellet:
                case Core::TileType::Tunnel: break;
            }
        }
    }
}

// Temporary placeholder: a yellow circle until the animated sprite arrives in the next task
void drawPlayer(const Core::PlayerSnapshot& player, const MazeLayout& layout)
{
    const auto tileSize{static_cast<float>(layout.tileSize)};
    const Vector2 centre{static_cast<float>(layout.originX) + (player.position.x * tileSize),
        static_cast<float>(layout.originY) + (player.position.y * tileSize)};
    DrawCircleV(centre, tileSize * 0.4f, YELLOW);
}

void drawHud(const Core::GameSnapshot& snapshot, ScreenSize screen)
{
    const int textY{(c_hudMargin - c_hudFontSize) / 2};
    DrawFPS(10, textY);

    const std::string timeText{std::format("Time: {:.2f}s", snapshot.totalTime)};
    DrawText(timeText.c_str(), 120, textY, c_hudFontSize, LIGHTGRAY);

    const std::string stateText{std::format("STATE: {}", Core::toString(snapshot.state))};
    const int stateWidth{MeasureText(stateText.c_str(), c_hudFontSize)};
    DrawText(stateText.c_str(), screen.width - stateWidth - 10, textY, c_hudFontSize, LIGHTGRAY);

    constexpr int tipFontSize{18};
    const int tipY{screen.height - c_hudMargin + ((c_hudMargin - tipFontSize) / 2)};
    DrawText("Controls: [WASD/Arrows] Move | [P] Pause/Resume | [R] Reset | [ESC] Quit", 10, tipY, tipFontSize, GRAY);
}

void drawStateOverlay(Core::GameState state, ScreenSize screen)
{
    const char* text{nullptr};
    Color color{WHITE};
    switch (state)
    {
        case Core::GameState::Playing: return;
        case Core::GameState::Paused:
            text = "PAUSED - PRESS [P] TO RESUME";
            color = ORANGE;
            break;
        case Core::GameState::GameOver:
            text = "GAME OVER";
            color = RED;
            break;
    }

    constexpr int fontSize{30};
    const int textWidth{MeasureText(text, fontSize)};
    const int x{(screen.width - textWidth) / 2};
    const int y{(screen.height - fontSize) / 2};
    // Dim backdrop keeps the message readable on top of the walls
    DrawRectangle(x - 16, y - 12, textWidth + 32, fontSize + 24, Fade(BLACK, 0.8f));
    DrawText(text, x, y, fontSize, color);
}

}  // namespace

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

    const ScreenSize screen{.width = GetScreenWidth(), .height = GetScreenHeight()};

    BeginDrawing();
    ClearBackground(BLACK);

    // A default snapshot has no maze, and the layout would divide by zero
    if (snapshot.mazeRows > 0 && snapshot.mazeCols > 0)
    {
        const MazeLayout layout{computeLayout(snapshot, screen)};
        drawMaze(snapshot, layout);
        drawPlayer(snapshot.player, layout);
    }
    drawHud(snapshot, screen);
    drawStateOverlay(snapshot.state, screen);

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

    for (const KeyBinding& binding : c_moveBindings)
    {
        if (IsKeyPressed(binding.key))
        {
            commands.push_back(binding.command);
        }
    }

    return commands;
}

}  // namespace LifeExe::Presentation
