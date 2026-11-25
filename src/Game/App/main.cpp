#include "Config.h"
#include "Game.h"
#include "HighScoreStorage.h"
#include "RaylibRenderer.h"
#include "SoundSystem.h"
#include "Timer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <print>

using namespace LifeExe;

namespace
{

// Relative to the working directory, next to config.json
constexpr const char* c_highScorePath{"highscore.json"};

bool isRoundOver(Core::GameState state)
{
    return state == Core::GameState::Victory || state == Core::GameState::GameOver;
}

// The file is written only when the record has grown, so an ordinary game never touches the disk
void saveHighScoreIfImproved(int currentHigh, int& savedHighScore)
{
    if (currentHigh > savedHighScore)
    {
        LifeExe::saveHighScore(c_highScorePath, currentHigh);
        savedHighScore = currentHigh;
    }
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
        int savedHighScore{LifeExe::loadHighScore(c_highScorePath)};
        game.setHighScore(savedHighScore);

        Presentation::RaylibRenderer renderer;
        renderer.init(config.width, config.height, config.title);

        Presentation::SoundSystem soundSystem;

        App::Timer timer;

        constexpr float fixedDt{1.0f / 60.0f};
        constexpr float maxFrameTime{0.25f};
        float accumulator{0.0f};

        // Esc in the main menu moves the core to Exit; the end screens stay up until the player chooses what next
        while (!renderer.shouldClose() && game.state() != Core::GameState::Exit)
        {
            // 1. Translate platform input into core commands
            for (const Core::Command command : renderer.processInput())
            {
                game.handleCommand(command);
            }

            const float frameTime{std::min(timer.tick(), maxFrameTime)};
            accumulator += frameTime;

            const Core::GameState stateBeforeUpdate{game.state()};
            while (accumulator >= fixedDt)
            {
                game.update(fixedDt);
                accumulator -= fixedDt;
            }

            // The core queues events across fixed steps, so one drain per frame plays everything this frame produced
            soundSystem.processEvents(game.takeEvents());

            // A finished round is the natural checkpoint for the record, so a crash later cannot lose it
            if (!isRoundOver(stateBeforeUpdate) && isRoundOver(game.state()))
            {
                saveHighScoreIfImproved(game.snapshot().highScore, savedHighScore);
            }

            renderer.render(game.snapshot());
        }

        // Closing the window or quitting mid-round skips the end screens, so the record is saved here as well
        saveHighScoreIfImproved(game.snapshot().highScore, savedHighScore);

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
