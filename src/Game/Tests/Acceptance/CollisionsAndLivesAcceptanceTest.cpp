#include <gtest/gtest.h>

#include <string_view>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "GridMover.h"
#include "Maze.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::Direction;
using Core::GameState;
using Core::GridMover;
using Core::GridPos;
using Core::Maze;
using Core::Mover;
using Core::RoundPhase;

namespace
{

constexpr float c_tolerance{1e-3f};

// Straight corridor where Ruby starts directly to the right of Chomper.
// One pellet sits on the left so tests can check that eaten pellets survive respawn.
constexpr std::string_view c_duelMap{"###########\n"
                                     "#..P..1...#\n"
                                     "#.###=###.#\n"
                                     "#.# 234 #.#\n"
                                     "###########\n"};

// Single pellet sits between Chomper and Ruby; Ruby is walled on the right so it must walk left onto the pellet.
constexpr std::string_view c_victoryBeforeDeathMap{"###########\n"
                                                   "#     P.1##\n"
                                                   "# ###=### #\n"
                                                   "# # 234 # #\n"
                                                   "###########\n"};

void advanceReady(Core::Game& game)
{
    while (game.state() == GameState::Playing && game.snapshot().roundPhase == RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
}

void triggerCollision(Core::Game& game)
{
    game.handleCommand(Command::MoveRight);
    for (int step{0}; step < 200 && game.snapshot().roundPhase == RoundPhase::Active; ++step)
    {
        game.update(Core::c_maxSubstep);
    }
}

void advanceDying(Core::Game& game)
{
    while (game.state() == GameState::Playing && game.snapshot().roundPhase == RoundPhase::PlayerDying)
    {
        game.update(Core::c_maxSubstep);
    }
}

void loseOneLife(Core::Game& game, int expectedLivesAfter)
{
    advanceReady(game);
    triggerCollision(game);
    ASSERT_EQ(game.snapshot().roundPhase, RoundPhase::PlayerDying);
    EXPECT_EQ(game.snapshot().lives, expectedLivesAfter);
    advanceDying(game);
}

}  // namespace

TEST(CollisionsAndLivesAcceptance, RoundPhaseToStringAndConstantsMatchContract)
{
    EXPECT_EQ(Core::toString(RoundPhase::Ready), "Ready");
    EXPECT_EQ(Core::toString(RoundPhase::Active), "Active");
    EXPECT_EQ(Core::toString(RoundPhase::PlayerDying), "PlayerDying");

    EXPECT_EQ(Core::c_startingLives, 3);
    EXPECT_GT(Core::c_readyDuration, 0.0f);
    EXPECT_GT(Core::c_dyingDuration, 0.0f);
}

TEST(CollisionsAndLivesAcceptance, ReadyPhaseFreezesEntitiesWhileAdvancingTotalTime)
{
    Core::Game game{Tests::makeReadyGame(Tests::c_smallLevel)};
    const Core::GameSnapshot initial{game.snapshot()};
    EXPECT_EQ(initial.lives, Core::c_startingLives);
    EXPECT_EQ(initial.roundPhase, RoundPhase::Ready);
    EXPECT_NEAR(initial.phaseTimeLeft, Core::c_readyDuration, c_tolerance);

    // Move commands during Ready are buffered for the first Active tick
    game.handleCommand(Command::MoveRight);
    game.update(Core::c_readyDuration - 0.25f);

    const Core::GameSnapshot duringReady{game.snapshot()};
    EXPECT_EQ(duringReady.roundPhase, RoundPhase::Ready);
    EXPECT_NEAR(duringReady.phaseTimeLeft, 0.25f, c_tolerance);
    EXPECT_DOUBLE_EQ(duringReady.totalTime, static_cast<double>(Core::c_readyDuration - 0.25f));
    EXPECT_FLOAT_EQ(duringReady.player.position.x, initial.player.position.x);
    EXPECT_FLOAT_EQ(duringReady.ghosts[0].position.x, initial.ghosts[0].position.x);

    // Crossing c_readyDuration enters Active; buffered MoveRight starts moving on the next step
    game.update(0.25f);
    EXPECT_EQ(game.snapshot().roundPhase, RoundPhase::Active);
    EXPECT_FLOAT_EQ(game.snapshot().phaseTimeLeft, 0.0f);

    game.update(0.125f);
    EXPECT_GT(game.snapshot().player.position.x, initial.player.position.x);
}

TEST(CollisionsAndLivesAcceptance, MoversCollideByCollisionTileAndHeadOnCrossing)
{
    const Maze maze{Maze::fromAscii(c_duelMap).value()};

    // Same cell centre -> collision
    const GridMover atSameCell{{1, 3}, Direction::Right};
    const GridMover otherSameCell{{1, 3}, Direction::Left};
    EXPECT_TRUE(Core::moversCollide(atSameCell, otherSameCell));

    // Head-on crossing at equal speeds: both pass progress 0.5 simultaneously and swap collision tiles
    GridMover leftToRight{{1, 3}, Direction::Right};
    GridMover rightToLeft{{1, 4}, Direction::Left};
    Direction reqRight{Direction::Right};
    Direction reqLeft{Direction::Left};

    leftToRight.update(maze, {.mover = Mover::Player, .speed = 1.0f, .dt = 0.4f}, reqRight);
    rightToLeft.update(maze, {.mover = Mover::Ghost, .speed = 1.0f, .dt = 0.4f}, reqLeft);
    EXPECT_FALSE(Core::moversCollide(leftToRight, rightToLeft));

    // Advance both by 0.2 -> progress = 0.6 each; collision tiles swapped to (1,4) and (1,3), but progress sum is 1.2 >= 1.0
    leftToRight.update(maze, {.mover = Mover::Player, .speed = 1.0f, .dt = 0.2f}, reqRight);
    rightToLeft.update(maze, {.mover = Mover::Ghost, .speed = 1.0f, .dt = 0.2f}, reqLeft);
    EXPECT_EQ(leftToRight.collisionTile(), (GridPos{1, 4}));
    EXPECT_EQ(rightToLeft.collisionTile(), (GridPos{1, 3}));
    EXPECT_TRUE(Core::moversCollide(leftToRight, rightToLeft));
}

TEST(CollisionsAndLivesAcceptance, HeadOnCollisionAtDifferentSpeedsIsDetected)
{
    const Maze maze{Maze::fromAscii(c_duelMap).value()};
    GridMover faster{{1, 3}, Direction::Right};
    GridMover slower{{1, 4}, Direction::Left};
    Direction reqRight{Direction::Right};
    Direction reqLeft{Direction::Left};

    faster.update(maze, {.mover = Mover::Player, .speed = 8.0f, .dt = 0.08f}, reqRight);
    slower.update(maze, {.mover = Mover::Ghost, .speed = 7.5f, .dt = 0.08f}, reqLeft);
    EXPECT_TRUE(Core::moversCollide(faster, slower));
}

TEST(CollisionsAndLivesAcceptance, CollisionDecrementsLivesImmediatelyAndRespawnsKeepingEatenPellets)
{
    Core::Game game{Tests::makePlayingGame(c_duelMap)};
    const int initialPellets{game.snapshot().remainingPellets};
    ASSERT_GT(initialPellets, 0);

    // Eat one pellet on the left first, then run right into Ruby
    game.handleCommand(Command::MoveLeft);
    game.update(1.2f / Core::c_playerSpeed);
    ASSERT_LT(game.snapshot().remainingPellets, initialPellets);

    triggerCollision(game);
    const Core::GameSnapshot dying{game.snapshot()};
    EXPECT_EQ(dying.state, GameState::Playing);
    EXPECT_EQ(dying.roundPhase, RoundPhase::PlayerDying);
    EXPECT_EQ(dying.lives, Core::c_startingLives - 1);
    EXPECT_NEAR(dying.phaseTimeLeft, Core::c_dyingDuration, c_tolerance);

    // Entities freeze and Move commands are ignored while PlayerDying
    const Core::Vec2f frozenPlayerPos{dying.player.position};
    game.handleCommand(Command::MoveLeft);
    game.update(Core::c_dyingDuration * 0.5f);
    EXPECT_EQ(game.snapshot().roundPhase, RoundPhase::PlayerDying);
    EXPECT_FLOAT_EQ(game.snapshot().player.position.x, frozenPlayerPos.x);

    // Finishing PlayerDying respawns player and ghosts in Ready without restoring eaten pellets
    advanceDying(game);
    const Core::GameSnapshot respawned{game.snapshot()};
    EXPECT_EQ(respawned.state, GameState::Playing);
    EXPECT_EQ(respawned.roundPhase, RoundPhase::Ready);
    EXPECT_NEAR(respawned.phaseTimeLeft, Core::c_readyDuration, c_tolerance);
    EXPECT_EQ(respawned.lives, Core::c_startingLives - 1);
    EXPECT_EQ(respawned.remainingPellets, dying.remainingPellets);
    EXPECT_EQ(respawned.score, dying.score);
    EXPECT_FLOAT_EQ(respawned.player.position.x, 3.5f);
    EXPECT_FLOAT_EQ(respawned.ghosts[0].position.x, 6.5f);
}

TEST(CollisionsAndLivesAcceptance, LosingLastLifeTransitionsToGameOverAfterDyingPhase)
{
    Core::Game game{Tests::makePlayingGame(c_duelMap)};

    for (int loss{0}; loss < Core::c_startingLives; ++loss)
    {
        loseOneLife(game, Core::c_startingLives - 1 - loss);
    }

    EXPECT_EQ(game.state(), GameState::GameOver);
    EXPECT_EQ(game.snapshot().lives, 0);

    // Reset restores lives, pellets and starts fresh in Ready
    game.handleCommand(Command::Reset);
    const Core::GameSnapshot resetSnap{game.snapshot()};
    EXPECT_EQ(resetSnap.state, GameState::Playing);
    EXPECT_EQ(resetSnap.roundPhase, RoundPhase::Ready);
    EXPECT_EQ(resetSnap.lives, Core::c_startingLives);
}

TEST(CollisionsAndLivesAcceptance, EatingLastPelletBeatsCollisionOnTheSameStep)
{
    Core::Game game{Tests::makePlayingGame(c_victoryBeforeDeathMap)};
    ASSERT_EQ(game.snapshot().remainingPellets, 1);

    // Chomper at (1, 6) moves right onto the last pellet at (1, 7) while Ruby at (1, 8) is forced left onto (1, 7)
    game.handleCommand(Command::MoveRight);
    game.update(1.0f / Core::c_playerSpeed);

    EXPECT_EQ(game.state(), GameState::Victory);
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives);
}
