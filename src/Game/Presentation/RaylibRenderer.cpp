#include "RaylibRenderer.h"

#include <raylib.h>

#include "SpriteAtlas.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <format>
#include <numbers>
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

Rectangle toRectangle(SpriteRect rect)
{
    return {static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.width), static_cast<float>(rect.height)};
}

void drawPellet(Rectangle tile, const Texture2D& spritesheet)
{
    DrawTexturePro(spritesheet, toRectangle(pelletSprite()), tile, {0.0f, 0.0f}, 0.0f, WHITE);
}

// The power pellet breathes so it stands out from regular pellets; totalTime freezes on pause, and so does the pulse
void drawPowerPellet(Rectangle tile, double totalTime, const Texture2D& spritesheet)
{
    constexpr float pulseHz{2.0f};
    constexpr float minScale{0.8f};
    const float phase{std::sin(static_cast<float>(totalTime) * 2.0f * std::numbers::pi_v<float> * pulseHz)};
    const float scale{minScale + ((1.0f - minScale) * 0.5f * (phase + 1.0f))};

    const float size{tile.width * scale};
    // Scaling around the tile centre keeps the sprite in place while it pulses
    const Rectangle dest{tile.x + (tile.width * 0.5f), tile.y + (tile.height * 0.5f), size, size};
    const Vector2 origin{size * 0.5f, size * 0.5f};
    DrawTexturePro(spritesheet, toRectangle(powerPelletSprite()), dest, origin, 0.0f, WHITE);
}

void drawMaze(const Core::GameSnapshot& snapshot, const MazeLayout& layout, const Texture2D& spritesheet)
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
                case Core::TileType::Pellet: drawPellet(tile, spritesheet); break;
                case Core::TileType::PowerPellet: drawPowerPellet(tile, snapshot.totalTime, spritesheet); break;
                case Core::TileType::Empty:
                case Core::TileType::Tunnel: break;
            }
        }
    }
}

// The atlas frames face right, so every other direction is a rotation of the same frames
float facingAngle(Core::Direction facing)
{
    switch (facing)
    {
        case Core::Direction::Right: return 0.0f;
        case Core::Direction::Down: return 90.0f;
        case Core::Direction::Left: return 180.0f;
        case Core::Direction::Up: return 270.0f;
        case Core::Direction::None: return 0.0f;
    }
    return 0.0f;
}

void drawPlayer(const Core::PlayerSnapshot& player, const MazeLayout& layout, const Texture2D& spritesheet)
{
    // animTime freezes while the player stands, so the mouth stops on the current frame
    constexpr float chomperFps{12.0f};
    const int frame{static_cast<int>(player.animTime * chomperFps)};

    const auto size{static_cast<float>(layout.tileSize)};
    const Rectangle dest{static_cast<float>(layout.originX) + (player.position.x * size),
        static_cast<float>(layout.originY) + (player.position.y * size), size, size};
    // Rotating around the tile centre keeps the sprite on the player's position for every facing
    const Vector2 origin{size * 0.5f, size * 0.5f};
    DrawTexturePro(spritesheet, toRectangle(playerMoveFrame(frame)), dest, origin, facingAngle(player.facing), WHITE);
}

Color ghostTint(Core::GhostId id)
{
    switch (id)
    {
        case Core::GhostId::Ruby: return RED;
        case Core::GhostId::Rosie: return PINK;
        case Core::GhostId::Indigo: return SKYBLUE;
        case Core::GhostId::Coral: return ORANGE;
    }
    return WHITE;
}

// One body sprite serves every ghost through the tint; the eyes are a separate layer so they keep their own colours
void drawGhost(const Core::GhostSnapshot& ghost, const MazeLayout& layout, double totalTime, const Texture2D& spritesheet)
{
    // totalTime freezes on pause, and so does the skirt
    constexpr double skirtPhasesPerSecond{6.0};
    const int phase{static_cast<int>(totalTime * skirtPhasesPerSecond)};

    const auto size{static_cast<float>(layout.tileSize)};
    const Rectangle dest{static_cast<float>(layout.originX) + (ghost.position.x * size),
        static_cast<float>(layout.originY) + (ghost.position.y * size), size, size};
    // position is the tile centre, so the sprite is anchored by its centre
    const Vector2 origin{size * 0.5f, size * 0.5f};
    DrawTexturePro(spritesheet, toRectangle(ghostBodyFrame(phase)), dest, origin, 0.0f, ghostTint(ghost.id));
    DrawTexturePro(spritesheet, toRectangle(ghostEyesFrame(ghost.direction)), dest, origin, 0.0f, WHITE);
}

void drawTextRightAligned(const std::string& text, int rightX, int y, int fontSize, Color color)
{
    DrawText(text.c_str(), rightX - MeasureText(text.c_str(), fontSize), y, fontSize, color);
}

// Top band: what the player plays for - score, record and game state
void drawTopHud(const Core::GameSnapshot& snapshot, ScreenSize screen)
{
    constexpr int sidePadding{10};
    const int textY{(c_hudMargin - c_hudFontSize) / 2};

    const std::string scoreText{std::format("SCORE: {}", snapshot.score)};
    DrawText(scoreText.c_str(), sidePadding, textY, c_hudFontSize, WHITE);

    const std::string highScoreText{std::format("HIGH SCORE: {}", snapshot.highScore)};
    const int highScoreWidth{MeasureText(highScoreText.c_str(), c_hudFontSize)};
    DrawText(highScoreText.c_str(), (screen.width - highScoreWidth) / 2, textY, c_hudFontSize, GOLD);

    const std::string stateText{std::format("STATE: {}", Core::toString(snapshot.state))};
    drawTextRightAligned(stateText, screen.width - sidePadding, textY, c_hudFontSize, LIGHTGRAY);
}

// Bottom band: progress and diagnostics on the first line, controls on the second
void drawBottomHud(const Core::GameSnapshot& snapshot, ScreenSize screen)
{
    constexpr int sidePadding{10};
    constexpr int fontSize{16};
    constexpr int lineGap{6};
    const int bandTop{screen.height - c_hudMargin};
    const int firstLineY{bandTop + ((c_hudMargin - (2 * fontSize) - lineGap) / 2)};
    const int secondLineY{firstLineY + fontSize + lineGap};

    const std::string pelletsText{std::format("PELLETS: {}", snapshot.remainingPellets)};
    DrawText(pelletsText.c_str(), sidePadding, firstLineY, fontSize, WHITE);

    const std::string diagnosticsText{std::format("FPS: {}  Time: {:.2f}s", GetFPS(), snapshot.totalTime)};
    drawTextRightAligned(diagnosticsText, screen.width - sidePadding, firstLineY, fontSize, LIGHTGRAY);

    DrawText("Controls: [WASD/Arrows] Move | [P] Pause/Resume | [R] Reset | [ESC] Quit", sidePadding, secondLineY, fontSize, GRAY);
}

void drawHud(const Core::GameSnapshot& snapshot, ScreenSize screen)
{
    drawTopHud(snapshot, screen);
    drawBottomHud(snapshot, screen);
}

// Victory covers the whole screen: the round is over, and the dimmed maze shows what was cleared
void drawVictoryOverlay(ScreenSize screen)
{
    DrawRectangle(0, 0, screen.width, screen.height, Fade(BLACK, 0.6f));

    constexpr const char* title{"VICTORY!"};
    constexpr int titleFontSize{48};
    const int titleWidth{MeasureText(title, titleFontSize)};
    const int titleY{(screen.height / 2) - titleFontSize};
    DrawText(title, (screen.width - titleWidth) / 2, titleY, titleFontSize, GOLD);

    constexpr const char* hint{"PRESS [R] TO RESTART"};
    constexpr int hintFontSize{24};
    const int hintWidth{MeasureText(hint, hintFontSize)};
    DrawText(hint, (screen.width - hintWidth) / 2, titleY + titleFontSize + 16, hintFontSize, RAYWHITE);
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
        case Core::GameState::Victory: drawVictoryOverlay(screen); return;
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
        // Textures need the GL context created by InitWindow; the path is relative to the working directory
        m_spritesheet = LoadTexture("assets/spritesheet.png");
        // Point filtering keeps pixel art crisp when a 16px tile is scaled up
        SetTextureFilter(m_spritesheet, TEXTURE_FILTER_POINT);
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
        // The texture lives in the GL context, so it must be freed before the window closes
        if (m_spritesheet.id != 0)
        {
            UnloadTexture(m_spritesheet);
        }
        m_spritesheet = {};
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
        drawMaze(snapshot, layout, m_spritesheet);
        drawPlayer(snapshot.player, layout, m_spritesheet);
        for (const Core::GhostSnapshot& ghost : snapshot.ghosts)
        {
            drawGhost(ghost, layout, snapshot.totalTime, m_spritesheet);
        }
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
