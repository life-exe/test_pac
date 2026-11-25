#pragma once

#include <memory>
#include <string_view>
#include <utility>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "Maze.h"
#include "RandomSource.h"
#include "Types.h"

namespace LifeExe::Tests
{

// Small test maze: 9 rows by 11 columns. It has everything the parser requires: the player start P,
// ghost starts 1..4, the ghost house door '=' and one pair of tunnels in row 5.
inline constexpr std::string_view c_smallLevel{"###########\n"
                                               "#....1....#\n"
                                               "#.###=###.#\n"
                                               "#.# 234 #.#\n"
                                               "#.#######.#\n"
                                               "T....P....T\n"
                                               "#o.......o#\n"
                                               "#.........#\n"
                                               "###########\n"};

// Creates a game on the given map, confirms past MainMenu and leaves it in Playing / RoundPhase::Ready.
inline Core::Game makeReadyGame(std::string_view asciiMap = c_smallLevel)
{
    Core::Game game{Core::Maze::fromAscii(asciiMap).value()};
    game.handleCommand(Core::Command::Confirm);
    return game;
}

// Creates a game on the given map with a custom random source, confirms past MainMenu,
// advances through Ready to Active and drains the RoundStarted event so tests start with an empty queue.
inline Core::Game makePlayingGame(std::string_view asciiMap, std::unique_ptr<Core::IRandomSource> random)
{
    Core::Game game{Core::Maze::fromAscii(asciiMap).value(), std::move(random)};
    game.handleCommand(Core::Command::Confirm);
    while (game.state() == Core::GameState::Playing && game.snapshot().roundPhase == Core::RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
    (void)game.takeEvents();
    return game;
}

// Creates a game on c_smallLevel with a custom random source and returns it Active with an empty event queue.
inline Core::Game makePlayingGame(std::unique_ptr<Core::IRandomSource> random)
{
    return makePlayingGame(c_smallLevel, std::move(random));
}

// Creates a game on the given map and returns it Active with an empty event queue.
inline Core::Game makePlayingGame(std::string_view asciiMap = c_smallLevel)
{
    return makePlayingGame(asciiMap, std::make_unique<Core::DefaultRandomSource>());
}

}  // namespace LifeExe::Tests
