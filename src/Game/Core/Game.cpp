#include "Game.h"

#include <utility>

#include "Level.h"

namespace LifeExe::Core
{

namespace
{

// Maps a movement command to its direction; any other command yields None
Direction toDirection(Command command)
{
    switch (command)
    {
        case Command::MoveUp: return Direction::Up;
        case Command::MoveDown: return Direction::Down;
        case Command::MoveLeft: return Direction::Left;
        case Command::MoveRight: return Direction::Right;
        case Command::None:
        case Command::Pause:
        case Command::Resume:
        case Command::TogglePause:
        case Command::Reset:
        case Command::Quit: return Direction::None;
    }
    return Direction::None;
}

}  // namespace

// The built-in level is a compile-time constant, so a parse failure is a programming error
Game::Game() : Game(Maze::fromAscii(c_classicLevel).value()) {}

Game::Game(Maze maze) : m_maze{std::move(maze)}, m_player{m_maze.playerSpawn()} {}

void Game::update(float dt)
{
    if (m_state == GameState::Playing)
    {
        m_totalTime += dt;
        m_player.update(m_maze, dt);
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

        case Command::None: break;
        case Command::MoveUp:
        case Command::MoveDown:
        case Command::MoveLeft:
        case Command::MoveRight:
            // A turn pressed during a pause must not fire after resume, so it is dropped, not queued
            if (m_state == GameState::Playing)
            {
                m_player.queueDirection(toDirection(command));
            }
            break;
    }
}

void Game::reset()
{
    m_state = GameState::Playing;
    m_totalTime = 0.0;
    m_player = Player{m_maze.playerSpawn()};
}

}  // namespace LifeExe::Core
