#include "Game.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

#include "GameConstants.h"
#include "Level.h"

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
        default: return Direction::None;
    }
}

GhostConfig makeGhostConfig(const Maze& maze, GhostId id, bool inHouse, float releaseDelay)
{
    return {
        .id = id, .spawn = maze.ghostSpawn(id), .scatterCorner = maze.scatterCorner(id), .inHouse = inHouse, .releaseDelay = releaseDelay};
}

// Order matches GhostId, so m_ghosts can be indexed by it; Ruby starts outside and the rest leave the house in turn
std::array<Ghost, 4> makeGhosts(const Maze& maze)
{
    return {Ghost{makeGhostConfig(maze, GhostId::Ruby, false, 0.0f)},
        Ghost{makeGhostConfig(maze, GhostId::Rosie, true, c_rosieReleaseDelay)},
        Ghost{makeGhostConfig(maze, GhostId::Indigo, true, c_indigoReleaseDelay)},
        Ghost{makeGhostConfig(maze, GhostId::Coral, true, c_coralReleaseDelay)}};
}

}  // namespace

// The built-in map must be valid, so a parse error here is a programming error
// and value() may throw
Game::Game() : Game{Maze::fromAscii(c_classicLevel).value()} {}

Game::Game(Maze maze) : m_initialMaze{std::move(maze)}, m_maze{m_initialMaze}, m_player{m_maze.playerSpawn()}, m_ghosts{makeGhosts(m_maze)}
{
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

        case Command::Reset: reset(); break;

        case Command::Quit: m_state = GameState::GameOver; break;

        case Command::MoveUp:
        case Command::MoveDown:
        case Command::MoveLeft:
        case Command::MoveRight:
            if (m_state == GameState::Playing)
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
    m_maze = m_initialMaze;
    m_player = Player{m_maze.playerSpawn()};
    m_ghosts = makeGhosts(m_maze);
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
        .remainingPellets = m_maze.remainingPellets()};
}

void Game::updateSubstep(float stepDt)
{
    m_player.update(m_maze, stepDt);
    for (Ghost& ghost : m_ghosts)
    {
        ghost.update(m_maze, stepDt);
    }
    consumePelletAtPlayer();
}

std::array<GhostSnapshot, 4> Game::ghostSnapshots() const
{
    std::array<GhostSnapshot, 4> snapshots{};
    for (std::size_t index{0}; index < m_ghosts.size(); ++index)
    {
        const Ghost& ghost{m_ghosts[index]};
        snapshots[index] = {.id = ghost.id(), .position = ghost.position(), .direction = ghost.direction()};
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
    }
    else if (eaten == TileType::PowerPellet)
    {
        m_score += c_powerPelletScore;
    }
    m_highScore = std::max(m_highScore, m_score);

    if (m_maze.remainingPellets() == 0)
    {
        m_state = GameState::Victory;
    }
}

}  // namespace LifeExe::Core
