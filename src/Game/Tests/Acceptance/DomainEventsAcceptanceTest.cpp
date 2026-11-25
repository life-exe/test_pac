#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

#include "Command.h"
#include "Game.h"
#include "GameConstants.h"
#include "GameEvents.h"
#include "Maze.h"
#include "RandomSource.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;
using Core::Command;
using Core::GameEvent;
using Core::GameState;
using Core::GhostMode;
using Core::IRandomSource;
using Core::RoundPhase;

namespace
{

class FixedRandomSource final : public IRandomSource
{
public:
    explicit FixedRandomSource(std::size_t index) : m_index{index} {}
    std::size_t nextIndex(std::size_t count) override { return m_index % count; }

private:
    std::size_t m_index{0};
};

// Isolated player corridor with two pellets and a power pellet so events arrive without ghost interference.
constexpr std::string_view c_pelletEventsMap{"###########\n"
                                             "#   1     #\n"
                                             "# ##=#### #\n"
                                             "# # 234 # #\n"
                                             "###########\n"
                                             "#  P..o   #\n"
                                             "###########\n"};

// Corridor with a power pellet and Ruby right behind it, plus a backup pellet so Victory does not trigger early.
constexpr std::string_view c_ghostEatEventMap{"###########\n"
                                              "#..Po.1...#\n"
                                              "# ###=### #\n"
                                              "# # 234 # #\n"
                                              "###########\n"};

// Short corridor where Ruby starts to the right of Chomper for quick life loss and GameOver.
constexpr std::string_view c_deathEventMap{"###########\n"
                                           "#   P 1   #\n"
                                           "# ##=#### #\n"
                                           "# # 234 # #\n"
                                           "###########\n"};

bool containsEvent(const std::vector<GameEvent>& events, GameEvent target)
{
    return std::ranges::find(events, target) != events.end();
}

void triggerCollision(Core::Game& game)
{
    while (game.state() == GameState::Playing && game.snapshot().roundPhase == RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
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

void advanceUntilRubyEaten(Core::Game& game)
{
    for (int step{0}; step < 80 && game.snapshot().ghosts[0].mode == GhostMode::Frightened; ++step)
    {
        game.update(Core::c_maxSubstep);
    }
}

void loseNonFinalLife(Core::Game& game)
{
    triggerCollision(game);
    const std::vector<GameEvent> hitEvents{game.takeEvents()};
    EXPECT_TRUE(containsEvent(hitEvents, GameEvent::PlayerLostLife));
    EXPECT_FALSE(containsEvent(hitEvents, GameEvent::GameOver));
    advanceDying(game);
    (void)game.takeEvents();
}

}  // namespace

TEST(DomainEventsAcceptance, GameEventToStringHasCanonicalNames)
{
    EXPECT_EQ(Core::toString(GameEvent::PelletEaten), "PelletEaten");
    EXPECT_EQ(Core::toString(GameEvent::PowerPelletEaten), "PowerPelletEaten");
    EXPECT_EQ(Core::toString(GameEvent::GhostEaten), "GhostEaten");
    EXPECT_EQ(Core::toString(GameEvent::PlayerLostLife), "PlayerLostLife");
    EXPECT_EQ(Core::toString(GameEvent::RoundStarted), "RoundStarted");
    EXPECT_EQ(Core::toString(GameEvent::Victory), "Victory");
    EXPECT_EQ(Core::toString(GameEvent::GameOver), "GameOver");
}

TEST(DomainEventsAcceptance, RoundStartedIsEmittedOnReadyToActiveAndTakeEventsDrainsQueue)
{
    Core::Game game{Tests::makeReadyGame()};
    EXPECT_TRUE(game.takeEvents().empty());

    game.update(Core::c_readyDuration - Core::c_maxSubstep);
    EXPECT_TRUE(game.takeEvents().empty());

    game.update(Core::c_maxSubstep);
    const std::vector<GameEvent> events{game.takeEvents()};
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0], GameEvent::RoundStarted);

    // Second call returns empty vector because the first drained the buffer
    EXPECT_TRUE(game.takeEvents().empty());
}

TEST(DomainEventsAcceptance, EventsAccumulateAcrossMultipleUpdatesUntilDrained)
{
    Core::Game game{Tests::makePlayingGame(c_pelletEventsMap)};
    game.handleCommand(Command::MoveRight);

    // Two consecutive updates without takeEvents() in between: both PelletEaten events must survive
    game.update(1.1f / Core::c_playerSpeed);
    game.update(1.0f / Core::c_playerSpeed);

    const std::vector<GameEvent> firstBatch{game.takeEvents()};
    ASSERT_EQ(firstBatch.size(), 2U);
    EXPECT_EQ(firstBatch[0], GameEvent::PelletEaten);
    EXPECT_EQ(firstBatch[1], GameEvent::PelletEaten);

    // Eating the last tile (a power pellet) emits PowerPelletEaten followed by Victory
    game.update(1.2f / Core::c_playerSpeed);
    const std::vector<GameEvent> finalBatch{game.takeEvents()};
    ASSERT_EQ(finalBatch.size(), 2U);
    EXPECT_EQ(finalBatch[0], GameEvent::PowerPelletEaten);
    EXPECT_EQ(finalBatch[1], GameEvent::Victory);
}

TEST(DomainEventsAcceptance, EatingFrightenedGhostEmitsGhostEaten)
{
    Core::Game game{Tests::makePlayingGame(c_ghostEatEventMap, std::make_unique<FixedRandomSource>(0))};
    game.handleCommand(Command::MoveRight);

    game.update(1.0f / Core::c_playerSpeed);
    ASSERT_EQ(game.snapshot().ghosts[0].mode, GhostMode::Frightened);
    const std::vector<GameEvent> powerEvents{game.takeEvents()};
    EXPECT_TRUE(containsEvent(powerEvents, GameEvent::PowerPelletEaten));

    advanceUntilRubyEaten(game);
    const std::vector<GameEvent> eatEvents{game.takeEvents()};
    EXPECT_TRUE(containsEvent(eatEvents, GameEvent::GhostEaten));
}

TEST(DomainEventsAcceptance, PlayerLostLifeFiresOnCollisionAndGameOverAfterLastDyingTimer)
{
    Core::Game game{Tests::makePlayingGame(c_deathEventMap)};

    // Lose the first two lives
    for (int loss{0}; loss < Core::c_startingLives - 1; ++loss)
    {
        loseNonFinalLife(game);
    }

    // Last life: PlayerLostLife fires immediately upon entering PlayerDying, GameOver has not fired yet
    triggerCollision(game);
    const std::vector<GameEvent> lastHitEvents{game.takeEvents()};
    EXPECT_TRUE(containsEvent(lastHitEvents, GameEvent::PlayerLostLife));
    EXPECT_FALSE(containsEvent(lastHitEvents, GameEvent::GameOver));

    // Once PlayerDying finishes at 0 lives, GameOver fires
    advanceDying(game);
    const std::vector<GameEvent> overEvents{game.takeEvents()};
    EXPECT_EQ(game.state(), GameState::GameOver);
    EXPECT_TRUE(containsEvent(overEvents, GameEvent::GameOver));
}

TEST(DomainEventsAcceptance, ResetClearsUnreadEvents)
{
    Core::Game game{Tests::makePlayingGame(c_pelletEventsMap)};
    game.handleCommand(Command::MoveRight);
    game.update(1.1f / Core::c_playerSpeed);

    // Do not drain events; Reset must discard stale events from the previous run
    game.handleCommand(Command::Reset);
    EXPECT_TRUE(game.takeEvents().empty());
}
