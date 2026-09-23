#include <algorithm>
#include <chrono>
#include <print>
#include <thread>
#include "ConsoleRenderer.h"
#include "Game.h"
#include "Timer.h"
#include <fstream>
#include "Config.h"

using namespace LifeExe;

namespace
{

void processInput(Core::Game& game, int frame)
{
    // Simulated input stream until real window/input provider is attached
    if (frame == 3)
    {
        std::println(">>> Input Event: Pause");
        game.handleCommand(Core::Command::Pause);
    }
    else if (frame == 6)
    {
        std::println(">>> Input Event: Resume");
        game.handleCommand(Core::Command::Resume);
    }
    else if (frame == 9)
    {
        std::println(">>> Input Event: Quit");
        game.handleCommand(Core::Command::Quit);
    }
}

}  // namespace

int main()
{
    std::ifstream configFile{"config.json"};
    const LifeExe::GameConfig config{configFile.is_open()
                                         ? LifeExe::parseConfig(configFile)
                                         : LifeExe::GameConfig{.title = "LifeExe Game Framework", .width = 800, .height = 600}};
    std::println("Starting {} ({}x{})...", config.title, config.width, config.height);

    Core::Game game;
    const Presentation::ConsoleRenderer renderer;
    App::Timer timer;

    // Simulation parameters: 60 updates per second
    constexpr float fixedDt{1.0f / 60.0f};
    constexpr float maxFrameTime{0.25f};  // Spiral of death clamp

    float accumulator{0.0f};
    int simulatedFrames{0};

    // Run demonstration loop for 10 frames
    while (game.state() != Core::GameState::GameOver && simulatedFrames < 10)
    {
        // 1. Measure real elapsed time and clamp against lag spikes
        const float frameTime{std::min(timer.tick(), maxFrameTime)};
        accumulator += frameTime;

        // 2. Poll and process input events
        processInput(game, simulatedFrames);

        // 3. Fixed timestep physics/logic update
        while (accumulator >= fixedDt)
        {
            game.update(fixedDt);
            accumulator -= fixedDt;
        }

        // 4. Render current frame
        renderer.render(game);

        ++simulatedFrames;

        // Emulate frame target ~60 FPS (16 ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::println("Game loop finished successfully.");
    return 0;
}
