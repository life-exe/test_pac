#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "Ghost.h"
#include "GhostStrategy.h"
#include "Level.h"
#include "Maze.h"
#include "RandomSource.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::Direction;
using Core::GameState;
using Core::Ghost;
using Core::GhostContext;
using Core::GhostId;
using Core::GhostMode;
using Core::GridPos;
using Core::IRandomSource;
using Core::Maze;
using Core::RoundPhase;

namespace
{

class FixedRandomSource final : public IRandomSource
{
public:
    explicit FixedRandomSource(std::size_t index) : m_index{index} {}

    std::size_t nextIndex(std::size_t count) override
    {
        EXPECT_GE(count, 1U);
        return m_index % count;
    }

private:
    std::size_t m_index{0};
};

// Corridor with a power pellet right next to Chomper, Ruby ahead, and extra
// pellets so Victory does not trigger early.
constexpr std::string_view c_buffCorridor{"###########\n"
                                          "#..Po..1..#\n"
                                          "# ###=### #\n"
                                          "# # 234 # #\n"
                                          "###########\n"};

// Isolated player room with a power pellet so wave-timer pause and expiry can
// be tested without collisions.
constexpr std::string_view c_buffWaveMap{"###########\n"
                                         "#....1....#\n"
                                         "#.###=###.#\n"
                                         "#.# 234 #.#\n"
                                         "###########\n"
                                         "T...Po....T\n"
                                         "###########\n"};

// Corridor with a power pellet and Ruby right behind it, plus a second power
// pellet further right.
constexpr std::string_view c_eatGhostMap{"###########\n"
                                         "#..Po.1o..#\n"
                                         "# ###=### #\n"
                                         "# # 234 # #\n"
                                         "###########\n"};

// Open 4-way cross at (2, 2) for deterministic Frightened random-choice tests
constexpr std::string_view c_crossMap{"#########\n"
                                      "##.####1#\n"
                                      "#...###=#\n"
                                      "##.#P234#\n"
                                      "#########\n"};

void expectAllGhostsInMode(const Core::GameSnapshot& snap, GhostMode expected)
{
    for (const Core::GhostSnapshot& ghost : snap.ghosts)
    {
        EXPECT_EQ(ghost.mode, expected);
    }
}

void advanceUntilRubyLeavesFrightened(Core::Game& game)
{
    for (int step{0}; step < 80 && game.snapshot().ghosts[0].mode == GhostMode::Frightened; ++step)
    {
        game.update(Core::c_maxSubstep);
    }
}

void verifyEatenReturnFromCorner(const Maze& maze, GridPos startTile)
{
    const GridPos homeTile{maze.ghostSpawn(GhostId::Indigo)};
    Ghost ruby{{.id = GhostId::Ruby,
                   .spawn = startTile,
                   .scatterCorner = maze.scatterCorner(GhostId::Ruby),
                   .inHouse = false,
                   .releaseDelay = 0.0f,
                   .homeTile = homeTile},
        Core::rubyStrategy};
    ruby.setMode(GhostMode::Eaten, maze, false);

    FixedRandomSource random{0};
    const GhostContext context{.playerPos = maze.playerSpawn(),
        .playerDir = Direction::None,
        .rubyPos = startTile,
        .ghostPos = startTile,
        .scatterCorner = ruby.scatterCorner()};

    bool revived{false};
    for (int step{0}; step < 1200; ++step)
    {
        ruby.update(maze, Core::c_maxSubstep, {.context = context, .random = random, .waveMode = GhostMode::Chase});
        if (ruby.mode() == GhostMode::Chase)
        {
            revived = true;
            break;
        }
    }
    EXPECT_TRUE(revived) << "Eaten ghost stalled starting from (" << startTile.row << ", " << startTile.col << ")";
}

}  // namespace

TEST(BuffsAndStateMachineAcceptance, GhostModeNamesAndConstantsMatchContract)
{
    EXPECT_EQ(Core::toString(GhostMode::Frightened), "Frightened");
    EXPECT_EQ(Core::toString(GhostMode::Eaten), "Eaten");

    EXPECT_GT(Core::c_frightenedDuration, 0.0f);
    EXPECT_GT(Core::c_frightenedFlashTime, 0.0f);
    EXPECT_LT(Core::c_frightenedFlashTime, Core::c_frightenedDuration);
    EXPECT_LT(Core::c_frightenedSpeedMultiplier, 1.0f);
    EXPECT_GT(Core::c_eatenSpeedMultiplier, 1.0f);
    EXPECT_EQ(Core::c_baseGhostEatScore, 200);
}

TEST(BuffsAndStateMachineAcceptance, PowerPelletActivatesFrightenedReversesMazeGhostsAndPausesWaveTimer)
{
    Core::Game game{Tests::makePlayingGame(c_buffWaveMap, std::make_unique<FixedRandomSource>(0))};

    // Let Ruby move right along row 1 first
    game.update(0.25f);
    ASSERT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Scatter);
    const Direction rubyDirBefore{game.snapshot().ghosts[0].direction};
    ASSERT_NE(rubyDirBefore, Direction::None);

    // Eat the power pellet at (5, 5)
    game.handleCommand(Command::MoveRight);
    game.update(1.0f / Core::c_playerSpeed);

    const Core::GameSnapshot frightSnap{game.snapshot()};
    EXPECT_NEAR(frightSnap.frightenedTimer, Core::c_frightenedDuration, 0.15f);
    expectAllGhostsInMode(frightSnap, GhostMode::Frightened);
    EXPECT_EQ(frightSnap.ghosts[0].direction, Core::opposite(rubyDirBefore));

    // While Frightened runs for 7s, the Scatter wave timer (7s) must stay paused;
    // after Frightened expires, ghosts return to Scatter without reversing
    game.update(Core::c_frightenedDuration);
    const Core::GameSnapshot afterExpiry{game.snapshot()};
    EXPECT_FLOAT_EQ(afterExpiry.frightenedTimer, 0.0f);
    expectAllGhostsInMode(afterExpiry, GhostMode::Scatter);

    // The remaining Scatter time (~6.6s) still has to elapse before Chase begins
    game.update(1.0f);
    EXPECT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Scatter);
    game.update(Core::c_scatterDuration);
    EXPECT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Chase);
}

TEST(BuffsAndStateMachineAcceptance, CollidingWithFrightenedGhostEatsItAwardsBonusAndLeavesEatenHarmless)
{
    Core::Game game{Tests::makePlayingGame(c_eatGhostMap, std::make_unique<FixedRandomSource>(0))};
    game.handleCommand(Command::MoveRight);

    // Eat the power pellet at (1, 4): Ruby becomes Frightened and reverses left
    // toward Chomper
    game.update(1.0f / Core::c_playerSpeed);
    ASSERT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Frightened);
    const int scoreAfterPowerPellet{game.snapshot().score};

    // Keep moving right to collide with Frightened Ruby
    advanceUntilRubyLeavesFrightened(game);

    const Core::GameSnapshot afterEat{game.snapshot()};
    EXPECT_EQ(afterEat.state, GameState::Playing);
    EXPECT_EQ(afterEat.roundPhase, RoundPhase::Active);
    EXPECT_EQ(afterEat.lives, Core::c_startingLives);
    EXPECT_EQ(afterEat.ghosts[0].mode, GhostMode::Eaten);
    EXPECT_GE(afterEat.score, scoreAfterPowerPellet + Core::c_baseGhostEatScore);

    // Eating a second power pellet must not turn an Eaten ghost back to
    // Frightened
    game.update(3.0f / Core::c_playerSpeed);
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives);
    EXPECT_NE(game.snapshot().ghosts[0].mode, GhostMode::Frightened);
}

TEST(BuffsAndStateMachineAcceptance, ComboDoublesScoreAcrossMultipleFrightenedGhosts)
{
    // Chomper steps up from (2, 4) onto 'o' at (1, 4) and stops against the top
    // wall. Ruby reverses left into Chomper (+200), revives in Scatter and
    // patrols the right side, while Rosie (t=3s) and Indigo (t=6s) exit
    // Frightened and pick Left with FixedRandomSource(0) (+400, +800).
    constexpr std::string_view c_comboMap{"###############\n"
                                          "#...o1........#\n"
                                          "#..#P#=####.#.#\n"
                                          "#..#.# 234#.#.#\n"
                                          "#..#.######.#.#\n"
                                          "#.....#.......#\n"
                                          "###############\n"};
    Core::Game game{Tests::makePlayingGame(c_comboMap, std::make_unique<FixedRandomSource>(0))};
    game.handleCommand(Command::MoveUp);
    game.update(1.0f / Core::c_playerSpeed);
    ASSERT_GT(game.snapshot().frightenedTimer, 0.0f);
    const int scoreAfterPellet{game.snapshot().score};

    // Ruby (+200) is eaten immediately; Rosie exits at t=3s in Frightened and
    // walks ~5 tiles to (1, 4) (+400) before Indigo leaves the house at t=6s
    const float frightenedSpeed{Core::c_ghostSpeed * Core::c_frightenedSpeedMultiplier};
    game.update(Core::c_rosieReleaseDelay + (5.5f / frightenedSpeed));

    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives);
    EXPECT_EQ(game.snapshot().score - scoreAfterPellet, 600);
}

TEST(BuffsAndStateMachineAcceptance, FrightenedGhostUsesInjectedRandomSourceDeterministically)
{
    const Maze maze{Maze::fromAscii(c_crossMap).value()};
    // At rest in centre (2, 2): open exits in priority order are [Up, Left, Down,
    // Right]
    Ghost ghost{{.id = GhostId::Ruby, .spawn = {2, 2}, .scatterCorner = {0, 0}, .inHouse = false, .releaseDelay = 0.0f}};
    ghost.setMode(GhostMode::Frightened, maze, false);

    FixedRandomSource pickUp{0};
    FixedRandomSource pickLeft{1};
    FixedRandomSource pickDown{2};
    FixedRandomSource pickRight{3};

    EXPECT_EQ(ghost.chooseRandomDirection(maze, pickUp), Direction::Up);
    EXPECT_EQ(ghost.chooseRandomDirection(maze, pickLeft), Direction::Left);
    EXPECT_EQ(ghost.chooseRandomDirection(maze, pickDown), Direction::Down);
    EXPECT_EQ(ghost.chooseRandomDirection(maze, pickRight), Direction::Right);
}

TEST(BuffsAndStateMachineAcceptance, EatenGhostReturnsToHouseAndRevivesOnClassicLevelWithoutStalling)
{
    const Maze classic{Maze::fromAscii(Core::c_classicLevel).value()};
    verifyEatenReturnFromCorner(classic, {1, 1});
    verifyEatenReturnFromCorner(classic, {1, 19});
    verifyEatenReturnFromCorner(classic, {19, 1});
    verifyEatenReturnFromCorner(classic, {19, 19});
}

TEST(BuffsAndStateMachineAcceptance, FrightenedTimerResetsOnReset)
{
    Core::Game game{Tests::makePlayingGame(c_buffCorridor, std::make_unique<FixedRandomSource>(0))};
    game.handleCommand(Command::MoveRight);
    game.update(1.0f / Core::c_playerSpeed);
    ASSERT_GT(game.snapshot().frightenedTimer, 0.0f);

    game.handleCommand(Command::Reset);
    EXPECT_FLOAT_EQ(game.snapshot().frightenedTimer, 0.0f);
    expectAllGhostsInMode(game.snapshot(), GhostMode::Scatter);
}
