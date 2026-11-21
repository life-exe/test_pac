#pragma once

#include <string_view>

namespace LifeExe::Core
{

// Compile-time constants are named c_camelCase so they are never mistaken for variables.
// Legend: # wall, . pellet, o power pellet, space empty, = ghost house door, T tunnel,
// P player start, 1..4 ghost starts (1 outside the house, 2..4 inside).
inline constexpr std::string_view c_classicLevel{"#####################\n"
                                                 "#o........#........o#\n"
                                                 "#.###.###.#.###.###.#\n"
                                                 "#...................#\n"
                                                 "#.###.#.#####.#.###.#\n"
                                                 "#.....#...#...#.....#\n"
                                                 "#####.###.#.###.#####\n"
                                                 "#####.....1.....#####\n"
                                                 "#####.####=####.#####\n"
                                                 "T.....# 2 3 4 #.....T\n"
                                                 "#####.#########.#####\n"
                                                 "#.........#.........#\n"
                                                 "#.###.###.#.###.###.#\n"
                                                 "#...................#\n"
                                                 "#.###.#.#####.#.###.#\n"
                                                 "#.....#...P...#.....#\n"
                                                 "#####.###.#.###.#####\n"
                                                 "#o........#........o#\n"
                                                 "#.###.###.#.###.###.#\n"
                                                 "#...................#\n"
                                                 "#####################\n"};

}  // namespace LifeExe::Core
