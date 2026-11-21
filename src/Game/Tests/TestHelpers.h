#pragma once

#include <string_view>

#include "Game.h"
#include "Maze.h"

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

// Creates a game on the given map and returns it ready to play
inline Core::Game makePlayingGame(std::string_view asciiMap = c_smallLevel)
{
    return Core::Game{Core::Maze::fromAscii(asciiMap).value()};
}

}  // namespace LifeExe::Tests
