#include "Game.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include "GameConstants.h"
#include "Level.h"
#include "GhostStrategy.h"

namespace LifeExe::Core
{

namespace
{

Direction toDirection(Command command)
{
    switch (command)
    {
        case Command::MoveUp: return Direction::Up;
        case Command::MoveDown: return Direction::Down;
        case Command::MoveLeft: return Direction::Left;
        case Command::MoveRight: return Direction::Right;
        case Command::Pause:
        case Command::Resume:
        case Command::TogglePause:
        case Command::Reset:
        case Command::Quit:
        case Command::Confirm:
        case Command::None: return Direction::None;
    }
    return Direction::None;
}

bool canResetFrom(GameState state)
{
    return state == GameState::Playing || state == GameState::Paused || state == GameState::Victory || state == GameState::GameOver;
}

bool canConfirmNewGame(GameState state)
{
    return state == GameState::MainMenu || state == GameState::Victory || state == GameState::GameOver;
}

}  // namespace

// The built-in map must be valid, so a parse error here is a programming error
// and value() may throw
Game::Game() : Game{Maze::fromAscii(c_classicLevel).value(), std::make_unique<DefaultRandomSource>()} {}

Game::Game(Maze maze) : Game{std::move(maze), std::make_unique<DefaultRandomSource>()} {}

Game::Game(Maze maze, std::unique_ptr<IRandomSource> random)
    : m_initialMaze{std::move(maze)}, m_maze{m_initialMaze}, m_player{m_maze.playerSpawn()}, m_ghosts{createGhosts(m_maze)},
      m_random{random ? std::move(random) : std::make_unique<DefaultRandomSource>()}
{
}

// Order matches GhostId, so m_ghosts can be indexed by it; Ruby starts outside and the rest leave the house in turn
std::array<Ghost, 4> Game::createGhosts(const Maze& maze)
{
    const GridPos houseCenter{maze.ghostSpawn(GhostId::Indigo)};
    return {
        Ghost{{.id = GhostId::Ruby,
                  .spawn = maze.ghostSpawn(GhostId::Ruby),
                  .scatterCorner = maze.scatterCorner(GhostId::Ruby),
                  .inHouse = false,
                  .releaseDelay = 0.0f,
                  .homeTile = houseCenter},
            rubyStrategy},
        Ghost{{.id = GhostId::Rosie,
                  .spawn = maze.ghostSpawn(GhostId::Rosie),
                  .scatterCorner = maze.scatterCorner(GhostId::Rosie),
                  .inHouse = true,
                  .releaseDelay = c_rosieReleaseDelay,
                  .homeTile = houseCenter},
            rosieStrategy},
        Ghost{{.id = GhostId::Indigo,
                  .spawn = maze.ghostSpawn(GhostId::Indigo),
                  .scatterCorner = maze.scatterCorner(GhostId::Indigo),
                  .inHouse = true,
                  .releaseDelay = c_indigoReleaseDelay,
                  .homeTile = houseCenter},
            indigoStrategy},
        Ghost{{.id = GhostId::Coral,
                  .spawn = maze.ghostSpawn(GhostId::Coral),
                  .scatterCorner = maze.scatterCorner(GhostId::Coral),
                  .inHouse = true,
                  .releaseDelay = c_coralReleaseDelay,
                  .homeTile = houseCenter},
            coralStrategy},
    };
}

void Game::update(float dt)
{
    if (m_state != GameState::Playing || dt < 0.0f)
    {
        return;
    }
    if (dt == 0.0f)
    {
        updateSubstep(0.0f);
        return;
    }

    m_totalTime += dt;
    float remaining{dt};
    while (remaining > 0.0f && m_state == GameState::Playing)
    {
        const float step{std::min(remaining, c_maxSubstep)};
        updateSubstep(step);
        remaining -= step;
    }
}

void Game::handleCommand(Command command)
{
    switch (command)
    {
        case Command::Pause:
            if (m_state == GameState::Playing)
            {
                m_state = GameState::Paused;
            }
            break;

        case Command::Resume:
            if (m_state == GameState::Paused)
            {
                m_state = GameState::Playing;
            }
            break;

        case Command::TogglePause:
            if (m_state == GameState::Playing)
            {
                m_state = GameState::Paused;
            }
            else if (m_state == GameState::Paused)
            {
                m_state = GameState::Playing;
            }
            break;

        case Command::Reset:
            if (canResetFrom(m_state))
            {
                reset();
            }
            break;

        case Command::Confirm:
            if (canConfirmNewGame(m_state))
            {
                reset();
            }
            break;

        case Command::Quit:
            if (m_state == GameState::MainMenu)
            {
                m_state = GameState::Exit;
            }
            else if (m_state != GameState::Exit)
            {
                m_state = GameState::MainMenu;
            }
            break;

        case Command::MoveUp:
        case Command::MoveDown:
        case Command::MoveLeft:
        case Command::MoveRight:
            if (m_state == GameState::Playing && m_roundPhase != RoundPhase::PlayerDying)
            {
                m_player.queueDirection(toDirection(command));
            }
            break;

        case Command::None: break;
    }
}

void Game::reset()
{
    m_state = GameState::Playing;
    m_totalTime = 0.0;
    m_score = 0;
    m_lives = c_startingLives;
    m_maze = m_initialMaze;
    m_events.clear();
    respawnRound();
}

void Game::setHighScore(int highScore)
{
    m_highScore = std::max(m_highScore, highScore);
}

GameSnapshot Game::snapshot() const
{
    return {.state = m_state,
        .totalTime = m_totalTime,
        .mazeRows = m_maze.rows(),
        .mazeCols = m_maze.cols(),
        .tiles = m_maze.tiles(),
        .player = {.position = m_player.position(),
            .direction = m_player.direction(),
            .facing = m_player.facing(),
            .isMoving = m_player.isMoving(),
            .animTime = m_player.animTime()},
        .ghosts = ghostSnapshots(),
        .score = m_score,
        .highScore = m_highScore,
        .remainingPellets = m_maze.remainingPellets(),
        .lives = m_lives,
        .roundPhase = m_roundPhase,
        .phaseTimeLeft = m_phaseTimer,
        .frightenedTimer = m_frightenedTimer};
}

void Game::updateSubstep(float stepDt)
{
    if (m_roundPhase != RoundPhase::Active)
    {
        advancePhaseTimer(stepDt);
        return;
    }

    m_player.update(m_maze, stepDt);

    const GridPos playerTile{m_player.tile()};
    const Direction playerDir{m_player.direction()};
    const GridPos rubyTile{m_ghosts[0].tile()};
    for (Ghost& ghost : m_ghosts)
    {
        const GhostContext context{.playerPos = playerTile,
            .playerDir = playerDir,
            .rubyPos = rubyTile,
            .ghostPos = ghost.tile(),
            .scatterCorner = ghost.scatterCorner()};
        ghost.update(m_maze, stepDt, {.context = context, .random = *m_random, .waveMode = m_waveMode});
    }

    consumePelletAtPlayer();
    if (m_state != GameState::Playing)
    {
        return;
    }

    checkGhostCollisions();
    if (m_roundPhase != RoundPhase::Active)
    {
        return;
    }

    if (m_frightenedTimer > 0.0f)
    {
        updateFrightenedTimer(stepDt);
    }
    else
    {
        updateWaveTimer(stepDt);
    }
}

void Game::updateWaveTimer(float stepDt)
{
    m_waveTimer -= stepDt;
    if (m_waveTimer > 0.0f)
    {
        return;
    }

    if (m_waveMode == GhostMode::Scatter)
    {
        m_waveMode = GhostMode::Chase;
        m_waveTimer = c_chaseDuration;
    }
    else
    {
        m_waveMode = GhostMode::Scatter;
        m_waveTimer = c_scatterDuration;
    }

    for (Ghost& ghost : m_ghosts)
    {
        if (ghost.mode() != GhostMode::Eaten && ghost.mode() != GhostMode::Frightened)
        {
            ghost.setMode(m_waveMode, m_maze, true);
        }
    }
}

std::array<GhostSnapshot, 4> Game::ghostSnapshots() const
{
    std::array<GhostSnapshot, 4> snapshots{};
    for (std::size_t index{0}; index < m_ghosts.size(); ++index)
    {
        const Ghost& ghost{m_ghosts[index]};
        snapshots[index] = {.id = ghost.id(), .position = ghost.position(), .direction = ghost.direction(), .mode = ghost.mode()};
    }
    return snapshots;
}

void Game::consumePelletAtPlayer()
{
    const TileType eaten{m_maze.consumeTileAt(m_player.tile())};
    if (eaten == TileType::Empty)
    {
        return;
    }

    if (eaten == TileType::Pellet)
    {
        m_score += c_pelletScore;
        m_events.push_back(GameEvent::PelletEaten);
    }
    else if (eaten == TileType::PowerPellet)
    {
        m_score += c_powerPelletScore;
        m_events.push_back(GameEvent::PowerPelletEaten);
        activateFrightened();
    }
    m_highScore = std::max(m_highScore, m_score);

    if (m_maze.remainingPellets() == 0)
    {
        m_state = GameState::Victory;
        m_events.push_back(GameEvent::Victory);
    }
}

void Game::advancePhaseTimer(float stepDt)
{
    m_phaseTimer -= stepDt;
    if (m_phaseTimer > 0.0f)
    {
        return;
    }

    m_phaseTimer = 0.0f;
    if (m_roundPhase == RoundPhase::Ready)
    {
        m_roundPhase = RoundPhase::Active;
        m_events.push_back(GameEvent::RoundStarted);
        return;
    }

    if (m_lives > 0)
    {
        respawnRound();
    }
    else
    {
        m_state = GameState::GameOver;
        m_events.push_back(GameEvent::GameOver);
    }
}

void Game::checkGhostCollisions()
{
    for (Ghost& ghost : m_ghosts)
    {
        if (ghost.mode() == GhostMode::Eaten || !moversCollide(m_player.mover(), ghost.mover()))
        {
            continue;
        }

        if (ghost.mode() == GhostMode::Frightened)
        {
            ghost.setMode(GhostMode::Eaten, m_maze, false);
            const int shift{std::clamp(m_eatenGhostCombo, 0, 3)};
            m_score += c_baseGhostEatScore * (1 << shift);
            m_highScore = std::max(m_highScore, m_score);
            ++m_eatenGhostCombo;
            m_events.push_back(GameEvent::GhostEaten);
            continue;
        }

        --m_lives;
        m_roundPhase = RoundPhase::PlayerDying;
        m_phaseTimer = c_dyingDuration;
        m_events.push_back(GameEvent::PlayerLostLife);
        return;
    }
}

void Game::respawnRound()
{
    m_waveMode = GhostMode::Scatter;
    m_waveTimer = c_scatterDuration;
    m_frightenedTimer = 0.0f;
    m_eatenGhostCombo = 0;
    m_roundPhase = RoundPhase::Ready;
    m_phaseTimer = c_readyDuration;
    m_player = Player{m_maze.playerSpawn()};
    m_ghosts = createGhosts(m_maze);
}

void Game::activateFrightened()
{
    m_frightenedTimer = c_frightenedDuration;
    m_eatenGhostCombo = 0;
    for (Ghost& ghost : m_ghosts)
    {
        if (ghost.mode() != GhostMode::Eaten)
        {
            ghost.setMode(GhostMode::Frightened, m_maze, true);
        }
    }
}

void Game::updateFrightenedTimer(float stepDt)
{
    m_frightenedTimer -= stepDt;
    if (m_frightenedTimer > 0.0f)
    {
        return;
    }

    m_frightenedTimer = 0.0f;
    m_eatenGhostCombo = 0;
    for (Ghost& ghost : m_ghosts)
    {
        if (ghost.mode() == GhostMode::Frightened)
        {
            ghost.setMode(m_waveMode, m_maze, false);
        }
    }
}

std::vector<GameEvent> Game::takeEvents()
{
    std::vector<GameEvent> events;
    events.swap(m_events);
    return events;
}

}  // namespace LifeExe::Core
