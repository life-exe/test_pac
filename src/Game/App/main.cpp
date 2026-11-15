#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <print>
#include <thread>
#include <vector>
#include "Config.h"
#include "ConsoleRenderer.h"
#include "Game.h"
#include "Timer.h"

using namespace LifeExe;

namespace
{

// Input layer only reports commands; delivering them to the core is the presenter's job
std::vector<Core::Command> processInput(int frame)
{
    // Simulated input stream until real window/input provider is attached
    if (frame == 3)
    {
        std::println(">>> Input Event: Pause");
        return {Core::Command::Pause};
    }
    if (frame == 6)
    {
        std::println(">>> Input Event: Resume");
        return {Core::Command::Resume};
    }
    if (frame == 9)
    {
        std::println(">>> Input Event: Quit");
        return {Core::Command::Quit};
    }
    return {};
}

}  // namespace

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

            // 2. Poll input and hand this frame's commands to the core
            for (const Core::Command command : processInput(simulatedFrames))
            {
                game.handleCommand(command);
            }

            // 3. Fixed timestep physics/logic update
            while (accumulator >= fixedDt)
            {
                game.update(fixedDt);
                accumulator -= fixedDt;
            }

            // 4. Render current frame from a state snapshot
            renderer.render(game.snapshot());

            ++simulatedFrames;

            // Emulate frame target ~60 FPS (16 ms)
            std::this_thread::sleep_for(std::chrono::milliseconds{16});
        }

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
