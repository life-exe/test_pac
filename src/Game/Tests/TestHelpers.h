#pragma once

#include <string_view>

#include "Game.h"
#include "GameConstants.h"
#include "Maze.h"
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

// Creates a game on the given map in Playing / RoundPhase::Ready (without skipping the Ready pause).
inline Core::Game makeReadyGame(std::string_view asciiMap = c_smallLevel)
{
    return Core::Game{Core::Maze::fromAscii(asciiMap).value()};
}

// Creates a game on the given map and advances through the initial Ready pause to RoundPhase::Active.
inline Core::Game makePlayingGame(std::string_view asciiMap = c_smallLevel)
{
    Core::Game game{makeReadyGame(asciiMap)};
    while (game.state() == Core::GameState::Playing && game.snapshot().roundPhase == Core::RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
    return game;
}

}  // namespace LifeExe::Tests
