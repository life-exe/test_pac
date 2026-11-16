#include "Config.h"
#include "Game.h"
#include "SfmlRenderer.h"
#include "Timer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <print>

using namespace LifeExe;

int main()
{
    try
    {
        std::ifstream configFile{"config.json"};
        const LifeExe::GameConfig config{configFile.is_open()
                                             ? LifeExe::parseConfig(configFile)
                                             : LifeExe::GameConfig{.title = "LifeExe Game Framework", .width = 800, .height = 600}};
        std::println("Starting {} ({}x{})...", config.title, config.width, config.height);

        Core::Game game;
        Presentation::SfmlRenderer renderer;
        if (!renderer.init(config.width, config.height, config.title))
        {
            std::println(stderr, "Failed to initialize SFML renderer!");
            return EXIT_FAILURE;
        }

        App::Timer timer;

        constexpr float fixedDt{1.0f / 60.0f};
        constexpr float maxFrameTime{0.25f};
        float accumulator{0.0f};

        while (!renderer.shouldClose() && game.state() != Core::GameState::GameOver)
        {
            for (const Core::Command command : renderer.processInput())
            {
                game.handleCommand(command);
            }
            const float frameTime{std::min(timer.tick(), maxFrameTime)};
            accumulator += frameTime;

            while (accumulator >= fixedDt)
            {
                game.update(fixedDt);
                accumulator -= fixedDt;
            }

            renderer.render(game.snapshot());
        }

        renderer.close();
        std::println("Game loop finished successfully.");
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
