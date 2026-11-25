#include "RaylibRenderer.h"

#include <raylib.h>

#include "GameConstants.h"
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

// The collapse lasts exactly the PlayerDying phase, so its frame follows the phase progress rather than animTime
int deathFrameIndex(float phaseTimeLeft)
{
    constexpr float deathFrames{4.0f};
    const float progress{1.0f - (phaseTimeLeft / Core::c_dyingDuration)};
    return static_cast<int>(progress * deathFrames);
}

void drawPlayer(const Core::GameSnapshot& snapshot, const MazeLayout& layout, const Texture2D& spritesheet)
{
    const Core::PlayerSnapshot& player{snapshot.player};
    const auto size{static_cast<float>(layout.tileSize)};
    const Rectangle dest{static_cast<float>(layout.originX) + (player.position.x * size),
        static_cast<float>(layout.originY) + (player.position.y * size), size, size};
    // Rotating around the tile centre keeps the sprite on the player's position for every facing
    const Vector2 origin{size * 0.5f, size * 0.5f};

    if (snapshot.roundPhase == Core::RoundPhase::PlayerDying)
    {
        // The collapse does not depend on the facing, so the death frames are drawn upright
        DrawTexturePro(spritesheet, toRectangle(playerDeathFrame(deathFrameIndex(snapshot.phaseTimeLeft))), dest, origin, 0.0f, WHITE);
        return;
    }

    // animTime freezes while the player stands, so the mouth stops on the current frame
    constexpr float chomperFps{12.0f};
    const int frame{static_cast<int>(player.animTime * chomperFps)};
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

// The last seconds of the buff blink as a warning; totalTime freezes on pause, and so does the blinking
bool isFrightenedFlashing(const Core::GameSnapshot& snapshot)
{
    constexpr double flashPeriod{0.2};  // seconds per blink phase
    if (snapshot.frightenedTimer <= 0.0f || snapshot.frightenedTimer >= Core::c_frightenedFlashTime)
    {
        return false;
    }
    return static_cast<int>(snapshot.totalTime / flashPeriod) % 2 == 1;
}

// One body sprite serves every ghost through the tint; the eyes are a separate layer so they keep their own colours
void drawGhost(const Core::GhostSnapshot& ghost, const Core::GameSnapshot& snapshot, const MazeLayout& layout, const Texture2D& spritesheet)
{
    const auto size{static_cast<float>(layout.tileSize)};
    const Rectangle dest{static_cast<float>(layout.originX) + (ghost.position.x * size),
        static_cast<float>(layout.originY) + (ghost.position.y * size), size, size};
    // position is the tile centre, so the sprite is anchored by its centre
    const Vector2 origin{size * 0.5f, size * 0.5f};
    const Rectangle eyes{toRectangle(ghostEyesFrame(ghost.direction))};

    switch (ghost.mode)
    {
        case Core::GhostMode::Frightened:
            // The frightened frame has its own colours and face, so it is drawn untinted and without the eyes layer
            DrawTexturePro(spritesheet, toRectangle(ghostFrightenedFrame(isFrightenedFlashing(snapshot))), dest, origin, 0.0f, WHITE);
            return;
        case Core::GhostMode::Eaten:
            // Only the eyes fly home, so the player sees at once that this ghost is harmless
            DrawTexturePro(spritesheet, eyes, dest, origin, 0.0f, WHITE);
            return;
        case Core::GhostMode::Scatter:
        case Core::GhostMode::Chase: break;
    }

    // totalTime freezes on pause, and so does the skirt
    constexpr double skirtPhasesPerSecond{6.0};
    const int phase{static_cast<int>(snapshot.totalTime * skirtPhasesPerSecond)};
    DrawTexturePro(spritesheet, toRectangle(ghostBodyFrame(phase)), dest, origin, 0.0f, ghostTint(ghost.id));
    DrawTexturePro(spritesheet, eyes, dest, origin, 0.0f, WHITE);
}

// In Ready every ghost stands on its spawn, so the lowest ghost marks the house floor:
// the text goes two rows below it, on the row right under the house wall
void drawReadyText(const Core::GameSnapshot& snapshot, const MazeLayout& layout)
{
    constexpr const char* text{"READY!"};
    float lowestGhostY{0.0f};
    for (const Core::GhostSnapshot& ghost : snapshot.ghosts)
    {
        lowestGhostY = std::max(lowestGhostY, ghost.position.y);
    }
    // A house at the very bottom of a map would push the text off the maze
    const float rowCentreY{std::min(lowestGhostY + 2.0f, static_cast<float>(snapshot.mazeRows) - 0.5f)};

    const int fontSize{std::max(10, layout.tileSize)};
    const int textWidth{MeasureText(text, fontSize)};
    const int x{layout.originX + (((snapshot.mazeCols * layout.tileSize) - textWidth) / 2)};
    const int y{layout.originY + static_cast<int>(rowCentreY * static_cast<float>(layout.tileSize)) - (fontSize / 2)};
    DrawText(text, x, y, fontSize, YELLOW);
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

// Icons at the atlas' own size stay pixel-crisp and match the 16px HUD font line;
// the row is anchored by its top centre, so a point is passed instead of loose coordinates that could be swapped
void drawLives(int lives, Vector2 topCentre, const Texture2D& spritesheet)
{
    constexpr auto iconSize{static_cast<float>(c_tileSize)};
    constexpr float iconGap{4.0f};
    const int count{std::max(0, lives)};
    const float rowWidth{(static_cast<float>(count) * (iconSize + iconGap)) - iconGap};
    const float startX{topCentre.x - (rowWidth / 2.0f)};
    for (int index{0}; index < count; ++index)
    {
        const Rectangle dest{startX + (static_cast<float>(index) * (iconSize + iconGap)), topCentre.y, iconSize, iconSize};
        DrawTexturePro(spritesheet, toRectangle(lifeIconSprite()), dest, {0.0f, 0.0f}, 0.0f, WHITE);
    }
}

// Bottom band: pellets, lives and diagnostics on the first line, controls on the second
void drawBottomHud(const Core::GameSnapshot& snapshot, ScreenSize screen, const Texture2D& spritesheet)
{
    constexpr int sidePadding{10};
    constexpr int fontSize{16};
    constexpr int lineGap{6};
    const int bandTop{screen.height - c_hudMargin};
    const int firstLineY{bandTop + ((c_hudMargin - (2 * fontSize) - lineGap) / 2)};
    const int secondLineY{firstLineY + fontSize + lineGap};

    const std::string pelletsText{std::format("PELLETS: {}", snapshot.remainingPellets)};
    DrawText(pelletsText.c_str(), sidePadding, firstLineY, fontSize, WHITE);

    // Integer centre keeps the icons on whole pixels, so point filtering does not smear them
    const int livesCentreX{screen.width / 2};
    drawLives(snapshot.lives, {static_cast<float>(livesCentreX), static_cast<float>(firstLineY)}, spritesheet);

    const std::string diagnosticsText{std::format("FPS: {}  Time: {:.2f}s", GetFPS(), snapshot.totalTime)};
    drawTextRightAligned(diagnosticsText, screen.width - sidePadding, firstLineY, fontSize, LIGHTGRAY);

    DrawText("Controls: [WASD/Arrows] Move | [P] Pause/Resume | [R] Reset | [ESC] Menu", sidePadding, secondLineY, fontSize, GRAY);
}

void drawHud(const Core::GameSnapshot& snapshot, ScreenSize screen, const Texture2D& spritesheet)
{
    drawTopHud(snapshot, screen);
    drawBottomHud(snapshot, screen, spritesheet);
}

// The menu and every overlay centre their lines the same way, so the screens stay visually aligned
void drawCenteredText(const char* text, ScreenSize screen, int y, int fontSize, Color color)
{
    DrawText(text, (screen.width - MeasureText(text, fontSize)) / 2, y, fontSize, color);
}

// Victory and Game Over cover the whole screen: the round is over, and the dimmed maze shows how it ended
void drawEndOverlay(ScreenSize screen, const char* title, Color titleColor)
{
    DrawRectangle(0, 0, screen.width, screen.height, Fade(BLACK, 0.6f));

    constexpr int titleFontSize{48};
    constexpr int hintFontSize{24};
    constexpr int lineGap{16};
    const int titleY{(screen.height / 2) - titleFontSize};
    const int playAgainY{titleY + titleFontSize + lineGap};
    drawCenteredText(title, screen, titleY, titleFontSize, titleColor);
    drawCenteredText("PRESS [ENTER] OR [R] TO PLAY AGAIN", screen, playAgainY, hintFontSize, RAYWHITE);
    drawCenteredText("[ESC] MAIN MENU", screen, playAgainY + hintFontSize + lineGap, hintFontSize - 4, LIGHTGRAY);
}

// Pause keeps the maze visible under a lighter veil, so the player can study the situation before resuming
void drawPauseOverlay(ScreenSize screen)
{
    DrawRectangle(0, 0, screen.width, screen.height, Fade(BLACK, 0.5f));

    constexpr int titleFontSize{40};
    constexpr int hintFontSize{20};
    constexpr int lineGap{16};
    const int titleY{(screen.height / 2) - titleFontSize};
    drawCenteredText("PAUSED", screen, titleY, titleFontSize, ORANGE);
    drawCenteredText("[P] RESUME   [R] RESTART   [ESC] MAIN MENU", screen, titleY + titleFontSize + lineGap, hintFontSize, RAYWHITE);
}

void drawStateOverlay(Core::GameState state, ScreenSize screen)
{
    switch (state)
    {
        case Core::GameState::Paused: drawPauseOverlay(screen); return;
        case Core::GameState::GameOver: drawEndOverlay(screen, "GAME OVER", RED); return;
        case Core::GameState::Victory: drawEndOverlay(screen, "VICTORY!", GOLD); return;
        case Core::GameState::Playing:
        case Core::GameState::MainMenu:
        case Core::GameState::Exit: return;
    }
}

// The maze, the entities, the HUD and the state overlay: everything drawn while a round exists
void drawRound(const Core::GameSnapshot& snapshot, ScreenSize screen, const Texture2D& spritesheet)
{
    // A default snapshot has no maze, and the layout would divide by zero
    if (snapshot.mazeRows > 0 && snapshot.mazeCols > 0)
    {
        const MazeLayout layout{computeLayout(snapshot, screen)};
        drawMaze(snapshot, layout, spritesheet);
        drawPlayer(snapshot, layout, spritesheet);
        // Ghosts vanish while the player collapses, so the death animation is the only thing moving on the field
        if (snapshot.roundPhase != Core::RoundPhase::PlayerDying)
        {
            for (const Core::GhostSnapshot& ghost : snapshot.ghosts)
            {
                drawGhost(ghost, snapshot, layout, spritesheet);
            }
        }
        if (snapshot.state == Core::GameState::Playing && snapshot.roundPhase == Core::RoundPhase::Ready)
        {
            drawReadyText(snapshot, layout);
        }
    }
    drawHud(snapshot, screen, spritesheet);
    drawStateOverlay(snapshot.state, screen);
}

// --- Main menu ---

void drawSprite(const Texture2D& spritesheet, SpriteRect sprite, Rectangle dest, Color tint)
{
    DrawTexturePro(spritesheet, toRectangle(sprite), dest, {0.0f, 0.0f}, 0.0f, tint);
}

struct GhostCard
{
    Core::GhostId id{Core::GhostId::Ruby};
    const char* name{""};
};

constexpr std::array c_ghostCards{
    GhostCard{.id = Core::GhostId::Ruby, .name = "Ruby"},
    GhostCard{.id = Core::GhostId::Rosie, .name = "Rosie"},
    GhostCard{.id = Core::GhostId::Indigo, .name = "Indigo"},
    GhostCard{.id = Core::GhostId::Coral, .name = "Coral"},
};

// Each ghost gets an equal slot across the screen, tinted like in the maze and named in its own colour
// The clock goes last, away from the int row, so the two numbers cannot be swapped at the call site
void drawGhostParade(ScreenSize screen, int top, const Texture2D& spritesheet, double clock)
{
    constexpr int iconSize{3 * c_tileSize};  // a whole multiple of the atlas tile keeps the pixel art crisp
    constexpr int nameFontSize{20};
    constexpr int nameGap{8};
    constexpr double skirtPhasesPerSecond{6.0};
    const int phase{static_cast<int>(clock * skirtPhasesPerSecond)};
    const int slotWidth{screen.width / static_cast<int>(c_ghostCards.size())};

    for (std::size_t index{0}; index < c_ghostCards.size(); ++index)
    {
        const GhostCard& card{c_ghostCards[index]};
        const int centreX{(slotWidth * static_cast<int>(index)) + (slotWidth / 2)};
        const int iconLeft{centreX - (iconSize / 2)};
        const Rectangle dest{
            static_cast<float>(iconLeft), static_cast<float>(top), static_cast<float>(iconSize), static_cast<float>(iconSize)};
        const Color tint{ghostTint(card.id)};
        drawSprite(spritesheet, ghostBodyFrame(phase), dest, tint);
        drawSprite(spritesheet, ghostEyesFrame(Core::Direction::Down), dest, WHITE);

        const int nameWidth{MeasureText(card.name, nameFontSize)};
        DrawText(card.name, centreX - (nameWidth / 2), top + iconSize + nameGap, nameFontSize, tint);
    }
}

struct ScoreRow
{
    SpriteRect sprite{};
    std::string points;
};

// The points come from the core constants, so the menu cannot drift from the real scoring
void drawScoreTable(ScreenSize screen, int top, const Texture2D& spritesheet)
{
    constexpr int iconSize{2 * c_tileSize};
    constexpr int iconGap{16};
    constexpr int fontSize{20};
    constexpr int rowHeight{40};
    constexpr int ghostScore{Core::c_baseGhostEatScore};
    const std::array rows{
        ScoreRow{.sprite = pelletSprite(), .points = std::format("{} PTS", Core::c_pelletScore)},
        ScoreRow{.sprite = powerPelletSprite(), .points = std::format("{} PTS", Core::c_powerPelletScore)},
        ScoreRow{.sprite = ghostFrightenedFrame(false),
            .points = std::format("{} / {} / {} / {} PTS", ghostScore, 2 * ghostScore, 4 * ghostScore, 8 * ghostScore)},
    };

    // The icons form one column and the points another, so the block is centred by its widest line
    int widestText{0};
    for (const ScoreRow& row : rows)
    {
        widestText = std::max(widestText, MeasureText(row.points.c_str(), fontSize));
    }
    const int blockLeft{(screen.width - (iconSize + iconGap + widestText)) / 2};

    for (std::size_t index{0}; index < rows.size(); ++index)
    {
        const int rowTop{top + (static_cast<int>(index) * rowHeight)};
        const Rectangle dest{
            static_cast<float>(blockLeft), static_cast<float>(rowTop), static_cast<float>(iconSize), static_cast<float>(iconSize)};
        drawSprite(spritesheet, rows[index].sprite, dest, WHITE);
        DrawText(rows[index].points.c_str(), blockLeft + iconSize + iconGap, rowTop + ((iconSize - fontSize) / 2), fontSize, WHITE);
    }
}

// The clock goes first, away from the int row, so the two numbers cannot be swapped at the call site
void drawStartPrompt(double clock, ScreenSize screen, int top)
{
    constexpr double blinkPeriod{0.5};  // seconds the prompt stays visible, then hidden
    constexpr int promptFontSize{28};
    constexpr int exitFontSize{20};
    constexpr int lineGap{12};
    if (static_cast<int>(clock / blinkPeriod) % 2 == 0)
    {
        drawCenteredText("PRESS [ENTER] TO START", screen, top, promptFontSize, WHITE);
    }
    drawCenteredText("[ESC] EXIT", screen, top + promptFontSize + lineGap, exitFontSize, GRAY);
}

// The core does not update in MainMenu, so totalTime stands still; the window clock animates the menu instead
void drawMainMenu(const Core::GameSnapshot& snapshot, ScreenSize screen, const Texture2D& spritesheet)
{
    constexpr int titleFontSize{56};
    constexpr int highScoreFontSize{24};
    const double clock{GetTime()};
    // Rows are placed by a share of the height, so the layout follows the window size from config.json
    const auto rowAt{[screen](float share) { return static_cast<int>(static_cast<float>(screen.height) * share); }};

    drawCenteredText("MAZE CHOMP", screen, rowAt(0.08f), titleFontSize, YELLOW);
    drawGhostParade(screen, rowAt(0.24f), spritesheet, clock);
    drawScoreTable(screen, rowAt(0.46f), spritesheet);
    const std::string highScoreText{std::format("HIGH SCORE: {}", snapshot.highScore)};
    drawCenteredText(highScoreText.c_str(), screen, rowAt(0.72f), highScoreFontSize, GOLD);
    drawStartPrompt(clock, screen, rowAt(0.82f));
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

    switch (snapshot.state)
    {
        case Core::GameState::MainMenu: drawMainMenu(snapshot, screen, m_spritesheet); break;
        // The main loop ends on this frame, so there is nothing worth drawing
        case Core::GameState::Exit: break;
        case Core::GameState::Playing:
        case Core::GameState::Paused:
        case Core::GameState::GameOver:
        case Core::GameState::Victory: drawRound(snapshot, screen, m_spritesheet); break;
    }

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

    // The core decides what Confirm means: start from the menu, play again after the round, or nothing
    if (IsKeyPressed(KEY_ENTER))
    {
        commands.push_back(Core::Command::Confirm);
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
