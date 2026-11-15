#include <cstdio>
#include <cstdlib>
#include <exception>
#include <print>
#include "ConsoleRenderer.h"
#include "Game.h"

int main()
{
    try
    {
        std::println("Starting Game...");

        LifeExe::Core::Game game;
        const LifeExe::Presentation::ConsoleRenderer renderer;

        // Simulate game loop: 5 frames at 16 ms dt (~60 FPS)
        constexpr float dt{0.016f};
        for (int frame{0}; frame < 5; ++frame)
        {
            if (game.state() != LifeExe::Core::GameState::Playing)
            {
                break;
            }

            game.update(dt);
            renderer.render(game.snapshot());
        }

        std::println("Game finished successfully.");
    }
    catch (const std::exception& e)
    {
        // Handlers use fputs: it cannot throw, so no exception escapes main
        std::fputs(e.what(), stderr);
        std::fputs("\n", stderr);
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::fputs("unknown error\n", stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
