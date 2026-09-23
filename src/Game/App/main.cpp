#include <print>
#include "Game.h"
#include "ConsoleRenderer.h"

int main()
{
    std::println("Starting Game...");

    Game::Core::Game game;
    const Game::Presentation::ConsoleRenderer renderer;

    // Simulate game loop: 5 frames at 16 ms dt (~60 FPS)
    const float dt = 0.016F;
    for (int frame = 0; frame < 5; ++frame)
    {
        if (game.state() != Game::Core::GameState::Playing)
        {
            break;
        }

        game.update(dt);
        renderer.render(game);
    }

    std::println("Game finished successfully.");
    return 0;
}
