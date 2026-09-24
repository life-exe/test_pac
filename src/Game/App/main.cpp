#include <algorithm>
#include <print>
#include "SfmlRenderer.h"
#include "Game.h"
#include "Timer.h"
#include <fstream>
#include "Config.h"

using namespace LifeExe;

int main()
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
        return 1;
    }
    App::Timer timer;

    // Simulation parameters: 60 updates per second
    constexpr float fixedDt{1.0f / 60.0f};
    constexpr float maxFrameTime{0.25f};  // Spiral of death clamp

    float accumulator{0.0f};

    while (!renderer.shouldClose() && game.state() != Core::GameState::GameOver)
    {
        renderer.processInput(game);
        const float frameTime{std::min(timer.tick(), maxFrameTime)};
        accumulator += frameTime;

        while (accumulator >= fixedDt)
        {
            game.update(fixedDt);
            accumulator -= fixedDt;
        }

        renderer.render(game);
    }
    renderer.close();
    std::println("Game loop finished successfully.");
    return 0;
}
