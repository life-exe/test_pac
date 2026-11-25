#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "Game.h"
#include "GameConstants.h"
#include "GameEvents.h"
#include "Maze.h"
#include "RandomSource.h"
#include "TestHelpers.h"
#include "Types.h"

using namespace LifeExe;

namespace LifeExe::Core
{

// gtest prints unknown enums as raw bytes; found by ADL, this makes a failed event queue comparison readable
void PrintTo(GameEvent event, std::ostream* out)
{
    *out << toString(event);
}

}  // namespace LifeExe::Core

namespace
{

// A dead-end corridor with three pellets to the right of the spawn: two regular ones and a power pellet.
// Ghost spawns and the door exist only because the parser requires them.
constexpr std::string_view c_corridorLevel{"######\n"
                                           "#1234#\n"
                                           "##=###\n"
                                           "#P..o#\n"
                                           "######\n"};

constexpr Core::GridPos c_firstPellet{3, 2};
constexpr Core::GridPos c_secondPellet{3, 3};
constexpr Core::GridPos c_powerPellet{3, 4};
constexpr int c_corridorPellets{3};
constexpr int c_corridorFullScore{(2 * Core::c_pelletScore) + Core::c_powerPelletScore};

constexpr float secondsFor(float tiles)
{
    return tiles / Core::c_playerSpeed;
}

Core::Game makeCorridorGame()
{
    Core::Game game{Tests::makePlayingGame(c_corridorLevel)};
    game.handleCommand(Core::Command::MoveRight);
    return game;
}

Core::TileType tileAt(const Core::GameSnapshot& snapshot, Core::GridPos pos)
{
    const std::size_t index{
        (static_cast<std::size_t>(pos.row) * static_cast<std::size_t>(snapshot.mazeCols)) + static_cast<std::size_t>(pos.col)};
    return snapshot.tiles[index];
}

// Every ghost receives the wave mode, including those still in the house
void expectAllGhostsInMode(const Core::Game& game, Core::GhostMode mode)
{
    for (const Core::GhostSnapshot& ghost : game.snapshot().ghosts)
    {
        EXPECT_EQ(ghost.mode, mode) << "ghost " << static_cast<int>(ghost.id);
    }
}

// Margin around a wave boundary, so the checks never land exactly on the switch
constexpr float c_waveMargin{0.25f};

// The player is walled off from the ghost house, so long wave checks never end in a collision
constexpr std::string_view c_walledOffLevel{"###########\n"
                                            "#....1....#\n"
                                            "#.###=###.#\n"
                                            "#.# 234 #.#\n"
                                            "###########\n"
                                            "T....P....T\n"
                                            "###########\n"};

// Ruby starts in a dead end whose only exit leads to the player, so moving right always ends in a collision.
// The pellet behind the player is never reached while moving right, so the map cannot be cleared before the
// collision, whatever the exact eating moment is.
constexpr std::string_view c_collisionLevel{"#######\n"
                                            "#.P..1#\n"
                                            "###=###\n"
                                            "# 234 #\n"
                                            "#######\n"};

constexpr Core::GridPos c_collisionPelletBehind{1, 1};
constexpr Core::GridPos c_collisionPelletAhead{1, 3};

// Upper bound for phase loops, so a broken phase fails the test instead of hanging it
constexpr int c_maxPhaseSteps{640};  // 10 seconds of c_maxSubstep

// Wave and release timers tick only in RoundPhase::Active, so the Ready pause after Reset is skipped first
void skipReadyPhase(Core::Game& game)
{
    while (game.state() == Core::GameState::Playing && game.snapshot().roundPhase == Core::RoundPhase::Ready)
    {
        game.update(Core::c_maxSubstep);
    }
}

// Advances in substeps while the game is Playing in the given phase; false if the phase never ends
bool runWhilePhase(Core::Game& game, Core::RoundPhase phase)
{
    for (int step{0}; step < c_maxPhaseSteps; ++step)
    {
        if (game.state() != Core::GameState::Playing || game.snapshot().roundPhase != phase)
        {
            return true;
        }
        game.update(Core::c_maxSubstep);
    }
    return false;
}

// Plays a round on c_collisionLevel until the player meets Ruby; stops on the substep of the collision
bool runIntoGhost(Core::Game& game)
{
    skipReadyPhase(game);
    game.handleCommand(Core::Command::MoveRight);
    return runWhilePhase(game, Core::RoundPhase::Active);
}

// A full death on c_collisionLevel: the collision and then the whole PlayerDying pause
bool loseLife(Core::Game& game)
{
    return runIntoGhost(game) && runWhilePhase(game, Core::RoundPhase::PlayerDying);
}

Core::Vec2f centreOf(Core::GridPos tile)
{
    return {static_cast<float>(tile.col) + 0.5f, static_cast<float>(tile.row) + 0.5f};
}

void expectSamePosition(Core::Vec2f actual, Core::Vec2f expected)
{
    EXPECT_FLOAT_EQ(actual.x, expected.x);
    EXPECT_FLOAT_EQ(actual.y, expected.y);
}

// Nothing in the round moves: the player and every ghost keep their position and direction
void expectEntitiesFrozen(const Core::GameSnapshot& before, const Core::GameSnapshot& after)
{
    expectSamePosition(after.player.position, before.player.position);
    EXPECT_EQ(after.player.direction, before.player.direction);
    for (std::size_t index{0}; index < before.ghosts.size(); ++index)
    {
        SCOPED_TRACE(testing::Message() << "ghost " << index);
        expectSamePosition(after.ghosts[index].position, before.ghosts[index].position);
        EXPECT_EQ(after.ghosts[index].direction, before.ghosts[index].direction);
    }
}

void expectAtSpawns(const Core::GameSnapshot& snapshot, const Core::Maze& maze)
{
    expectSamePosition(snapshot.player.position, centreOf(maze.playerSpawn()));
    for (const Core::GhostSnapshot& ghost : snapshot.ghosts)
    {
        SCOPED_TRACE(testing::Message() << "ghost " << static_cast<int>(ghost.id));
        expectSamePosition(ghost.position, centreOf(maze.ghostSpawn(ghost.id)));
    }
}

// Always picks the same candidate index, so Frightened choices are reproducible
class FixedRandomSource final : public Core::IRandomSource
{
public:
    explicit FixedRandomSource(std::size_t index) : m_index{index} {}

    std::size_t nextIndex(std::size_t count) override { return count == 0 ? 0 : m_index % count; }

private:
    std::size_t m_index{0};
};

Core::Game makeFixedRandomGame(std::string_view asciiMap)
{
    return Tests::makePlayingGame(asciiMap, std::make_unique<FixedRandomSource>(0));
}

const Core::GhostSnapshot& ghostOf(const Core::GameSnapshot& snapshot, Core::GhostId id)
{
    return snapshot.ghosts[static_cast<std::size_t>(id)];
}

// Advances in substeps until the predicate holds for the snapshot; `before` keeps the snapshot of the step
// that made it true, so a test can compare the state just before and just after one event
template <typename Predicate>
bool stepUntil(Core::Game& game, Predicate predicate, Core::GameSnapshot& before, int maxSteps = c_maxPhaseSteps)
{
    for (int step{0}; step < maxSteps; ++step)
    {
        if (predicate(game.snapshot()))
        {
            return true;
        }
        before = game.snapshot();
        game.update(Core::c_maxSubstep);
    }
    return predicate(game.snapshot());
}

bool isFrightenedActive(const Core::GameSnapshot& snapshot)
{
    return snapshot.frightenedTimer > 0.0f;
}

bool isFrightenedOver(const Core::GameSnapshot& snapshot)
{
    return snapshot.frightenedTimer <= 0.0f;
}

bool isRubyEaten(const Core::GameSnapshot& snapshot)
{
    return ghostOf(snapshot, Core::GhostId::Ruby).mode == Core::GhostMode::Eaten;
}

// A ring of corridors without dead ends for the ghosts, so a reversal can only come from a mode change;
// the player has a separate row with a power pellet right ahead and no way to the ghosts
constexpr std::string_view c_frightenedRingLevel{"###########\n"
                                                 "#....1....#\n"
                                                 "#.###=###.#\n"
                                                 "#.# 234 #.#\n"
                                                 "#.#######.#\n"
                                                 "#.........#\n"
                                                 "###########\n"
                                                 "#.Po......#\n"
                                                 "###########\n"};

// Ruby walks left towards the player; the corridor between them is empty, so only the ghost changes the score.
// A second power pellet waits further right to be eaten while Ruby is still Eaten
constexpr std::string_view c_eatGhostLevel{"############\n"
                                           "#.Po    o.1#\n"
                                           "##=#########\n"
                                           "#234########\n"
                                           "############\n"};

constexpr Core::GridPos c_secondPowerPellet{1, 8};

// The player eats the power pellet above the door and stops in the dead end on the left, where Ruby is caught.
// The revived Ruby scatters to the ring on the right and never comes back, while Frightened Rosie leaves the
// house, takes the first candidate (Left) and walks into the player
constexpr std::string_view c_comboLevel{"##########\n"
                                        "#1 oP....#\n"
                                        "###=# ## #\n"
                                        "#423#    #\n"
                                        "##########\n"};

}  // namespace

TEST(GameCoreTest, InitialState)
{
    const Core::Game game;
    EXPECT_EQ(game.state(), Core::GameState::MainMenu);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);
}

// makePlayingGame has already spent the Ready pause, so time is measured as a difference
TEST(GameCoreTest, AccumulatesTotalTimeWhilePlaying)
{
    Core::Game game{Tests::makePlayingGame()};
    const double before{game.totalTime()};
    game.update(0.5f);
    game.update(0.25f);

    EXPECT_DOUBLE_EQ(game.totalTime() - before, 0.75);
}

TEST(GameCoreTest, SnapshotCopiesCoreState)
{
    Core::Game game{Tests::makePlayingGame()};
    const double before{game.totalTime()};
    game.update(0.5f);

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(snapshot.totalTime - before, 0.5);
}

TEST(GameCoreTest, GameStateToString)
{
    EXPECT_EQ(Core::toString(Core::GameState::Playing), "Playing");
    EXPECT_EQ(Core::toString(Core::GameState::Paused), "Paused");
    EXPECT_EQ(Core::toString(Core::GameState::GameOver), "GameOver");
    EXPECT_EQ(Core::toString(Core::GameState::Victory), "Victory");
    EXPECT_EQ(Core::toString(Core::GameState::MainMenu), "MainMenu");
    EXPECT_EQ(Core::toString(Core::GameState::Exit), "Exit");
}

TEST(GameCoreTest, HandlesPauseAndResumeCommands)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::Pause);
    EXPECT_EQ(game.state(), Core::GameState::Paused);

    const double pausedAt{game.totalTime()};
    game.update(1.0f);
    EXPECT_DOUBLE_EQ(game.totalTime() - pausedAt, 0.0);

    game.handleCommand(Core::Command::Resume);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    const double resumedAt{game.totalTime()};
    game.update(0.25f);
    EXPECT_DOUBLE_EQ(game.totalTime() - resumedAt, 0.25);
}

TEST(GameCoreTest, HandlesTogglePauseCommand)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::TogglePause);
    EXPECT_EQ(game.state(), Core::GameState::Paused);

    const double pausedAt{game.totalTime()};
    game.update(1.0f);
    EXPECT_DOUBLE_EQ(game.totalTime() - pausedAt, 0.0);

    game.handleCommand(Core::Command::TogglePause);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    // The main menu is not a round, so it cannot be paused or resumed
    game.handleCommand(Core::Command::Quit);
    game.handleCommand(Core::Command::TogglePause);
    EXPECT_EQ(game.state(), Core::GameState::MainMenu);
}

TEST(GameCoreTest, HandlesResetCommand)
{
    Core::Game game{Tests::makePlayingGame()};
    game.update(1.5f);
    game.handleCommand(Core::Command::Pause);

    // Reset starts a new game, so the clock restarts from zero rather than from the old value
    game.handleCommand(Core::Command::Reset);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_DOUBLE_EQ(game.totalTime(), 0.0);

    const double before{game.totalTime()};
    game.update(0.5f);
    EXPECT_DOUBLE_EQ(game.totalTime() - before, 0.5);
}

TEST(GameCoreTest, HandlesQuitCommand)
{
    Core::Game game{Tests::makePlayingGame()};
    game.handleCommand(Core::Command::Quit);
    EXPECT_EQ(game.state(), Core::GameState::MainMenu);

    game.handleCommand(Core::Command::Quit);
    EXPECT_EQ(game.state(), Core::GameState::Exit);
}

TEST(GameCoreTest, CommandToString)
{
    EXPECT_EQ(Core::toString(Core::Command::None), "None");
    EXPECT_EQ(Core::toString(Core::Command::Pause), "Pause");
    EXPECT_EQ(Core::toString(Core::Command::Resume), "Resume");
    EXPECT_EQ(Core::toString(Core::Command::TogglePause), "TogglePause");
    EXPECT_EQ(Core::toString(Core::Command::Reset), "Reset");
    EXPECT_EQ(Core::toString(Core::Command::Quit), "Quit");
    EXPECT_EQ(Core::toString(Core::Command::MoveUp), "MoveUp");
    EXPECT_EQ(Core::toString(Core::Command::MoveDown), "MoveDown");
    EXPECT_EQ(Core::toString(Core::Command::MoveLeft), "MoveLeft");
    EXPECT_EQ(Core::toString(Core::Command::MoveRight), "MoveRight");
    EXPECT_EQ(Core::toString(Core::Command::Confirm), "Confirm");
}

// --- Pellets, score and victory ---

TEST(GameScoreTest, EatingPelletAddsPelletScore)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(1.0f));

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.score, Core::c_pelletScore);
    EXPECT_EQ(snapshot.highScore, Core::c_pelletScore);
    EXPECT_EQ(snapshot.remainingPellets, c_corridorPellets - 1);
    EXPECT_EQ(tileAt(snapshot, c_firstPellet), Core::TileType::Empty);
}

TEST(GameScoreTest, EatingPowerPelletAddsPowerPelletScore)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(2.0f));
    ASSERT_EQ(game.snapshot().score, 2 * Core::c_pelletScore);

    game.update(secondsFor(1.0f));
    EXPECT_EQ(game.snapshot().score, c_corridorFullScore);
    EXPECT_EQ(tileAt(game.snapshot(), c_powerPellet), Core::TileType::Empty);
}

TEST(GameScoreTest, HighScoreFollowsScore)
{
    Core::Game game{makeCorridorGame()};
    for (int step{0}; step < c_corridorPellets; ++step)
    {
        game.update(secondsFor(1.0f));
        const Core::GameSnapshot snapshot{game.snapshot()};
        EXPECT_EQ(snapshot.highScore, snapshot.score) << "step " << step;
    }
}

TEST(GameScoreTest, SetHighScoreKeepsLargerRecord)
{
    Core::Game game{makeCorridorGame()};
    game.setHighScore(500);
    EXPECT_EQ(game.snapshot().highScore, 500);

    game.setHighScore(100);
    EXPECT_EQ(game.snapshot().highScore, 500);
}

TEST(GameScoreTest, SetHighScoreBelowScoreIsOvertaken)
{
    Core::Game game{makeCorridorGame()};
    game.setHighScore(15);

    game.update(secondsFor(1.0f));
    EXPECT_EQ(game.snapshot().score, 10);
    EXPECT_EQ(game.snapshot().highScore, 15);

    game.update(secondsFor(1.0f));
    EXPECT_EQ(game.snapshot().score, 20);
    EXPECT_EQ(game.snapshot().highScore, 20);
}

TEST(GameScoreTest, ResetRestoresPelletsAndKeepsHighScore)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(2.0f));
    ASSERT_EQ(game.snapshot().score, 2 * Core::c_pelletScore);

    game.handleCommand(Core::Command::Reset);

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.score, 0);
    EXPECT_EQ(snapshot.highScore, 2 * Core::c_pelletScore);
    EXPECT_EQ(snapshot.remainingPellets, c_corridorPellets);
    EXPECT_EQ(tileAt(snapshot, c_firstPellet), Core::TileType::Pellet);
    EXPECT_EQ(tileAt(snapshot, c_secondPellet), Core::TileType::Pellet);
}

TEST(GameScoreTest, NoPelletsEatenWhilePaused)
{
    Core::Game game{makeCorridorGame()};
    game.handleCommand(Core::Command::Pause);
    game.update(secondsFor(3.0f));

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.score, 0);
    EXPECT_EQ(snapshot.remainingPellets, c_corridorPellets);
}

TEST(GameVictoryTest, EatingLastPelletWinsGame)
{
    Core::Game game{makeCorridorGame()};
    for (int step{0}; step < c_corridorPellets - 1; ++step)
    {
        game.update(secondsFor(1.0f));
        EXPECT_EQ(game.state(), Core::GameState::Playing) << "step " << step;
    }

    game.update(secondsFor(1.0f));
    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Victory);
    EXPECT_EQ(snapshot.remainingPellets, 0);
    EXPECT_EQ(snapshot.score, c_corridorFullScore);
}

TEST(GameVictoryTest, VictoryFreezesGame)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(3.0f));
    ASSERT_EQ(game.state(), Core::GameState::Victory);
    const double finishTime{game.totalTime()};

    game.update(1.0f);
    EXPECT_EQ(game.state(), Core::GameState::Victory);
    EXPECT_DOUBLE_EQ(game.totalTime(), finishTime);
    EXPECT_EQ(game.snapshot().score, c_corridorFullScore);

    game.handleCommand(Core::Command::Reset);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    EXPECT_EQ(game.snapshot().remainingPellets, c_corridorPellets);
    EXPECT_EQ(game.snapshot().highScore, c_corridorFullScore);
}

// --- Scatter and Chase waves ---

TEST(GameWaveTest, GhostModeToString)
{
    EXPECT_EQ(Core::toString(Core::GhostMode::Scatter), "Scatter");
    EXPECT_EQ(Core::toString(Core::GhostMode::Chase), "Chase");
}

TEST(GameWaveTest, SwitchesScatterChaseScatter)
{
    // The player stands still behind a wall: no pellet is eaten, no ghost reaches the player, the game stays in Playing
    Core::Game game{Tests::makePlayingGame(c_walledOffLevel)};
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    game.update(Core::c_scatterDuration - c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);

    game.update(Core::c_chaseDuration - (2.0f * c_waveMargin));
    expectAllGhostsInMode(game, Core::GhostMode::Chase);

    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);
    EXPECT_EQ(game.state(), Core::GameState::Playing);
    // A death would restart the wave, so the sequence above is only valid without one
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives);
}

TEST(GameWaveTest, ResetRestartsScatterWave)
{
    Core::Game game{Tests::makePlayingGame(c_walledOffLevel)};
    game.update(Core::c_scatterDuration + c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);

    game.handleCommand(Core::Command::Reset);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    // The wave timer restarts too: after the Ready pause a full Scatter wave passes before Chase
    skipReadyPhase(game);
    game.update(Core::c_scatterDuration - c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);

    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);
}

TEST(GameVictoryTest, LargeStepEatsEveryPelletOnTheWay)
{
    Core::Game game{makeCorridorGame()};
    // One frame crosses three pellet tiles; only substeps let the core see each of them
    game.update(secondsFor(3.0f));

    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.remainingPellets, 0);
    EXPECT_EQ(snapshot.score, c_corridorFullScore);
    EXPECT_EQ(snapshot.state, Core::GameState::Victory);
    EXPECT_EQ(tileAt(snapshot, c_firstPellet), Core::TileType::Empty);
    EXPECT_EQ(tileAt(snapshot, c_secondPellet), Core::TileType::Empty);
    EXPECT_EQ(tileAt(snapshot, c_powerPellet), Core::TileType::Empty);
}

// --- Round phases, collisions and lives ---

TEST(GameRoundTest, RoundPhaseToString)
{
    EXPECT_EQ(Core::toString(Core::RoundPhase::Ready), "Ready");
    EXPECT_EQ(Core::toString(Core::RoundPhase::Active), "Active");
    EXPECT_EQ(Core::toString(Core::RoundPhase::PlayerDying), "PlayerDying");
}

TEST(GameRoundTest, ReadyPhaseFreezesEntitiesWhileTimeRuns)
{
    Core::Game game{Tests::makeReadyGame()};
    const Core::GameSnapshot atStart{game.snapshot()};
    EXPECT_EQ(atStart.state, Core::GameState::Playing);
    EXPECT_EQ(atStart.roundPhase, Core::RoundPhase::Ready);
    EXPECT_EQ(atStart.lives, Core::c_startingLives);
    EXPECT_FLOAT_EQ(atStart.phaseTimeLeft, Core::c_readyDuration);

    // A queued command does not start the player before the pause is over
    game.handleCommand(Core::Command::MoveRight);
    game.update(Core::c_readyDuration - 0.25f);
    const Core::GameSnapshot lateInReady{game.snapshot()};
    EXPECT_EQ(lateInReady.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(lateInReady.phaseTimeLeft, 0.25f);
    EXPECT_DOUBLE_EQ(lateInReady.totalTime, Core::c_readyDuration - 0.25f);
    expectEntitiesFrozen(atStart, lateInReady);

    game.update(0.5f);
    const Core::GameSnapshot inActive{game.snapshot()};
    EXPECT_EQ(inActive.roundPhase, Core::RoundPhase::Active);
    EXPECT_GT(inActive.player.position.x, atStart.player.position.x);
    const auto rubyIndex{static_cast<std::size_t>(Core::GhostId::Ruby)};
    EXPECT_NE(inActive.ghosts[rubyIndex].position.x, atStart.ghosts[rubyIndex].position.x);
}

TEST(GameRoundTest, GhostCollisionCostsLifeAndFreezesEntities)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(runIntoGhost(game));

    // runIntoGhost stops on the very substep of the collision, so the life is lost at once
    const Core::GameSnapshot atDeath{game.snapshot()};
    EXPECT_EQ(atDeath.state, Core::GameState::Playing);
    EXPECT_EQ(atDeath.roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_EQ(atDeath.lives, Core::c_startingLives - 1);
    EXPECT_FLOAT_EQ(atDeath.phaseTimeLeft, Core::c_dyingDuration);

    // Movement commands are ignored while the player dies
    game.handleCommand(Core::Command::MoveLeft);
    game.update(0.5f);
    const Core::GameSnapshot meanwhile{game.snapshot()};
    EXPECT_EQ(meanwhile.roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_EQ(meanwhile.lives, Core::c_startingLives - 1);
    EXPECT_NEAR(meanwhile.totalTime - atDeath.totalTime, 0.5, 1e-6);
    expectEntitiesFrozen(atDeath, meanwhile);
}

TEST(GameRoundTest, RespawnAfterDyingKeepsPelletsAndScore)
{
    const Core::Maze maze{Core::Maze::fromAscii(c_collisionLevel).value()};
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(runIntoGhost(game));
    const Core::GameSnapshot atDeath{game.snapshot()};
    ASSERT_GT(atDeath.score, 0);

    ASSERT_TRUE(runWhilePhase(game, Core::RoundPhase::PlayerDying));
    const Core::GameSnapshot respawned{game.snapshot()};
    EXPECT_EQ(respawned.state, Core::GameState::Playing);
    EXPECT_EQ(respawned.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(respawned.phaseTimeLeft, Core::c_readyDuration);
    EXPECT_EQ(respawned.lives, Core::c_startingLives - 1);
    EXPECT_EQ(respawned.score, atDeath.score);
    EXPECT_EQ(respawned.highScore, atDeath.highScore);
    EXPECT_EQ(respawned.remainingPellets, atDeath.remainingPellets);
    EXPECT_EQ(tileAt(respawned, c_collisionPelletAhead), Core::TileType::Empty);
    EXPECT_EQ(tileAt(respawned, c_collisionPelletBehind), Core::TileType::Pellet);
    expectAtSpawns(respawned, maze);
}

TEST(GameRoundTest, LosingLastLifeEndsGame)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(loseLife(game));
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives - 1);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    ASSERT_TRUE(loseLife(game));
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives - 2);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    // The last collision still plays the death pause before the game ends
    ASSERT_TRUE(runIntoGhost(game));
    EXPECT_EQ(game.snapshot().lives, 0);
    EXPECT_EQ(game.snapshot().roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_EQ(game.state(), Core::GameState::Playing);

    ASSERT_TRUE(runWhilePhase(game, Core::RoundPhase::PlayerDying));
    EXPECT_EQ(game.state(), Core::GameState::GameOver);
    EXPECT_EQ(game.snapshot().lives, 0);

    const double overTime{game.totalTime()};
    game.update(1.0f);
    EXPECT_DOUBLE_EQ(game.totalTime(), overTime);
}

TEST(GameRoundTest, ResetRestoresLivesAndReadyPhase)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(loseLife(game));
    ASSERT_EQ(game.snapshot().lives, Core::c_startingLives - 1);

    game.handleCommand(Core::Command::Reset);
    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_EQ(snapshot.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(snapshot.phaseTimeLeft, Core::c_readyDuration);
    EXPECT_EQ(snapshot.lives, Core::c_startingLives);
    EXPECT_EQ(snapshot.score, 0);
}

// --- Frightened buff and Eaten ghosts ---

TEST(GameBuffTest, GhostModeToStringBuffs)
{
    EXPECT_EQ(Core::toString(Core::GhostMode::Frightened), "Frightened");
    EXPECT_EQ(Core::toString(Core::GhostMode::Eaten), "Eaten");
}

TEST(GameBuffTest, PowerPelletFrightensAndReversesOnlyOutsideHouse)
{
    Core::Game game{makeFixedRandomGame(c_frightenedRingLevel)};
    // Rosie is already on her way out, so a reversal inside the house would show
    game.update(Core::c_rosieReleaseDelay + 0.1f);
    ASSERT_EQ(ghostOf(game.snapshot(), Core::GhostId::Rosie).direction, Core::Direction::Right);

    game.handleCommand(Core::Command::MoveRight);
    Core::GameSnapshot before{};
    ASSERT_TRUE(stepUntil(game, isFrightenedActive, before));

    const Core::GameSnapshot after{game.snapshot()};
    expectAllGhostsInMode(game, Core::GhostMode::Frightened);
    EXPECT_NEAR(after.frightenedTimer, Core::c_frightenedDuration, Core::c_maxSubstep);
    EXPECT_EQ(ghostOf(after, Core::GhostId::Ruby).direction, Core::opposite(ghostOf(before, Core::GhostId::Ruby).direction));
    EXPECT_EQ(ghostOf(after, Core::GhostId::Rosie).direction, ghostOf(before, Core::GhostId::Rosie).direction);
}

TEST(GameBuffTest, FrightenedPausesWaveAndExpiresWithoutReverse)
{
    Core::Game game{makeFixedRandomGame(c_frightenedRingLevel)};
    const double activeStart{game.totalTime()};
    game.handleCommand(Core::Command::MoveRight);
    Core::GameSnapshot before{};
    ASSERT_TRUE(stepUntil(game, isFrightenedActive, before));
    const auto waveElapsed{static_cast<float>(before.totalTime - activeStart)};

    ASSERT_TRUE(stepUntil(game, isFrightenedOver, before));
    const Core::GameSnapshot expired{game.snapshot()};
    // The buff outlasted the Scatter wave; with a running wave timer the ghosts would come back in Chase
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);
    // The ring has no dead ends, so the only possible reversal would come from the expiry itself
    EXPECT_NE(ghostOf(expired, Core::GhostId::Ruby).direction, Core::opposite(ghostOf(before, Core::GhostId::Ruby).direction));

    // The paused wave resumes with exactly the Scatter time it had left
    game.update(Core::c_scatterDuration - waveElapsed - c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Scatter);
    game.update(2.0f * c_waveMargin);
    expectAllGhostsInMode(game, Core::GhostMode::Chase);
    EXPECT_EQ(game.snapshot().lives, Core::c_startingLives);
}

TEST(GameBuffTest, EatingFrightenedGhostAndEatenSafety)
{
    Core::Game game{makeFixedRandomGame(c_eatGhostLevel)};
    Core::GameSnapshot before{};
    // The player waits until Ruby is close, so the chase after the power pellet is short
    ASSERT_TRUE(stepUntil(
        game, [](const Core::GameSnapshot& snapshot) { return ghostOf(snapshot, Core::GhostId::Ruby).position.x < 5.0f; }, before));
    game.handleCommand(Core::Command::MoveRight);
    ASSERT_TRUE(stepUntil(game, isFrightenedActive, before));

    ASSERT_TRUE(stepUntil(game, isRubyEaten, before));
    const Core::GameSnapshot atEat{game.snapshot()};
    EXPECT_EQ(atEat.score - before.score, Core::c_baseGhostEatScore);
    EXPECT_EQ(atEat.lives, Core::c_startingLives);
    EXPECT_EQ(atEat.roundPhase, Core::RoundPhase::Active);

    // A second power pellet frightens the house ghosts again, but the eyes keep flying home
    ASSERT_TRUE(stepUntil(
        game, [](const Core::GameSnapshot& snapshot) { return tileAt(snapshot, c_secondPowerPellet) == Core::TileType::Empty; }, before));
    const Core::GameSnapshot secondBuff{game.snapshot()};
    EXPECT_EQ(ghostOf(secondBuff, Core::GhostId::Ruby).mode, Core::GhostMode::Eaten);
    for (const Core::GhostId id : {Core::GhostId::Rosie, Core::GhostId::Indigo, Core::GhostId::Coral})
    {
        EXPECT_EQ(ghostOf(secondBuff, id).mode, Core::GhostMode::Frightened) << "ghost " << static_cast<int>(id);
    }
    EXPECT_NEAR(secondBuff.frightenedTimer, Core::c_frightenedDuration, Core::c_maxSubstep);

    // On the way back the eyes fly through the player without a collision
    bool crossedPlayer{false};
    ASSERT_TRUE(stepUntil(
        game,
        [&crossedPlayer](const Core::GameSnapshot& snapshot)
        {
            const Core::GhostSnapshot& ruby{ghostOf(snapshot, Core::GhostId::Ruby)};
            crossedPlayer = crossedPlayer || (ruby.mode == Core::GhostMode::Eaten && ruby.position.x < snapshot.player.position.x);
            return ruby.mode != Core::GhostMode::Eaten || snapshot.lives < Core::c_startingLives;
        },
        before));
    const Core::GameSnapshot revived{game.snapshot()};
    EXPECT_TRUE(crossedPlayer);
    EXPECT_EQ(revived.lives, Core::c_startingLives);
    EXPECT_EQ(revived.roundPhase, Core::RoundPhase::Active);
    // The revived ghost takes the current wave mode, not Frightened
    EXPECT_EQ(ghostOf(revived, Core::GhostId::Ruby).mode, Core::GhostMode::Scatter);
}

TEST(GameBuffTest, ComboDoublesWithinOneFrightened)
{
    Core::Game game{makeFixedRandomGame(c_comboLevel)};
    game.handleCommand(Core::Command::MoveLeft);
    Core::GameSnapshot before{};
    ASSERT_TRUE(stepUntil(game, isFrightenedActive, before));

    ASSERT_TRUE(stepUntil(game, isRubyEaten, before));
    EXPECT_EQ(game.snapshot().score - before.score, Core::c_baseGhostEatScore);

    ASSERT_TRUE(stepUntil(
        game, [](const Core::GameSnapshot& snapshot) { return ghostOf(snapshot, Core::GhostId::Rosie).mode == Core::GhostMode::Eaten; },
        before));
    const Core::GameSnapshot secondEat{game.snapshot()};
    EXPECT_EQ(secondEat.score - before.score, 2 * Core::c_baseGhostEatScore);
    EXPECT_EQ(secondEat.lives, Core::c_startingLives);
    EXPECT_GT(secondEat.frightenedTimer, 0.0f);
}

// --- Screen state machine: one test per non-empty cell of the transition table ---

namespace
{

// GameOver needs a map where the player meets a ghost; every other state uses the pellet corridor
std::string_view levelFor(Core::GameState state)
{
    return state == Core::GameState::GameOver ? c_collisionLevel : c_corridorLevel;
}

// Builds a game standing in the given screen state. Every state inside a round already has some score,
// so a new game is distinguishable from simply continuing the old one
Core::Game makeGameIn(Core::GameState state)
{
    switch (state)
    {
        case Core::GameState::MainMenu: return Core::Game{Core::Maze::fromAscii(levelFor(state)).value()};
        case Core::GameState::Playing:
        {
            Core::Game game{makeCorridorGame()};
            game.update(secondsFor(1.0f));
            return game;
        }
        case Core::GameState::Paused:
        {
            Core::Game game{makeGameIn(Core::GameState::Playing)};
            game.handleCommand(Core::Command::Pause);
            return game;
        }
        case Core::GameState::Victory:
        {
            Core::Game game{makeCorridorGame()};
            game.update(secondsFor(static_cast<float>(c_corridorPellets)));
            return game;
        }
        case Core::GameState::GameOver:
        {
            Core::Game game{Tests::makeReadyGame(levelFor(state))};
            for (int life{0}; life < Core::c_startingLives; ++life)
            {
                loseLife(game);
            }
            return game;
        }
        case Core::GameState::Exit:
        {
            Core::Game game{makeGameIn(Core::GameState::MainMenu)};
            game.handleCommand(Core::Command::Quit);
            return game;
        }
    }
    return Core::Game{};
}

int initialPelletsFor(Core::GameState state)
{
    return Core::Maze::fromAscii(levelFor(state)).value().remainingPellets();
}

// A new game: the round restarts from the Ready pause with a clean score, full lives and every pellet back
void expectNewGame(const Core::Game& game, Core::GameState from)
{
    const Core::GameSnapshot snapshot{game.snapshot()};
    EXPECT_EQ(snapshot.state, Core::GameState::Playing);
    EXPECT_EQ(snapshot.roundPhase, Core::RoundPhase::Ready);
    EXPECT_FLOAT_EQ(snapshot.phaseTimeLeft, Core::c_readyDuration);
    EXPECT_DOUBLE_EQ(snapshot.totalTime, 0.0);
    EXPECT_EQ(snapshot.score, 0);
    EXPECT_EQ(snapshot.lives, Core::c_startingLives);
    EXPECT_EQ(snapshot.remainingPellets, initialPelletsFor(from));
}

// Everything the player has achieved in the round stays as it was
void expectSameProgress(const Core::GameSnapshot& before, const Core::GameSnapshot& after)
{
    EXPECT_EQ(after.score, before.score);
    EXPECT_EQ(after.highScore, before.highScore);
    EXPECT_DOUBLE_EQ(after.totalTime, before.totalTime);
    EXPECT_EQ(after.roundPhase, before.roundPhase);
    EXPECT_FLOAT_EQ(after.phaseTimeLeft, before.phaseTimeLeft);
    EXPECT_EQ(after.lives, before.lives);
    EXPECT_EQ(after.remainingPellets, before.remainingPellets);
}

// A screen change that is not a new game only switches the state and keeps the round untouched
void expectScreenChange(Core::GameState from, Core::Command command, Core::GameState expected)
{
    Core::Game game{makeGameIn(from)};
    ASSERT_EQ(game.state(), from);
    const Core::GameSnapshot before{game.snapshot()};

    game.handleCommand(command);
    EXPECT_EQ(game.state(), expected);
    expectSameProgress(before, game.snapshot());
}

void expectStartsNewGame(Core::GameState from, Core::Command command)
{
    Core::Game game{makeGameIn(from)};
    ASSERT_EQ(game.state(), from);
    ASSERT_GT(game.snapshot().score, 0);
    const int highScore{game.snapshot().highScore};

    game.handleCommand(command);
    expectNewGame(game, from);
    EXPECT_EQ(game.snapshot().highScore, highScore);
}

constexpr std::array c_endStates{Core::GameState::Victory, Core::GameState::GameOver};

}  // namespace

TEST(GameScreenTest, MainMenuConfirmStartsNewGame)
{
    Core::Game game{makeGameIn(Core::GameState::MainMenu)};
    constexpr int savedHighScore{500};
    game.setHighScore(savedHighScore);

    game.handleCommand(Core::Command::Confirm);
    expectNewGame(game, Core::GameState::MainMenu);
    EXPECT_EQ(game.snapshot().highScore, savedHighScore);
}

TEST(GameScreenTest, MainMenuQuitExits)
{
    expectScreenChange(Core::GameState::MainMenu, Core::Command::Quit, Core::GameState::Exit);
}

TEST(GameScreenTest, PlayingTogglePausePauses)
{
    expectScreenChange(Core::GameState::Playing, Core::Command::TogglePause, Core::GameState::Paused);
}

TEST(GameScreenTest, PlayingPausePauses)
{
    expectScreenChange(Core::GameState::Playing, Core::Command::Pause, Core::GameState::Paused);
}

TEST(GameScreenTest, PlayingResetStartsNewGame)
{
    expectStartsNewGame(Core::GameState::Playing, Core::Command::Reset);
}

TEST(GameScreenTest, PlayingQuitReturnsToMainMenu)
{
    expectScreenChange(Core::GameState::Playing, Core::Command::Quit, Core::GameState::MainMenu);
}

TEST(GameScreenTest, PausedTogglePauseResumes)
{
    expectScreenChange(Core::GameState::Paused, Core::Command::TogglePause, Core::GameState::Playing);
}

TEST(GameScreenTest, PausedResumeResumes)
{
    expectScreenChange(Core::GameState::Paused, Core::Command::Resume, Core::GameState::Playing);
}

TEST(GameScreenTest, PausedResetStartsNewGame)
{
    expectStartsNewGame(Core::GameState::Paused, Core::Command::Reset);
}

TEST(GameScreenTest, PausedQuitReturnsToMainMenu)
{
    expectScreenChange(Core::GameState::Paused, Core::Command::Quit, Core::GameState::MainMenu);
}

TEST(GameScreenTest, EndScreenConfirmStartsNewGame)
{
    for (const Core::GameState endState : c_endStates)
    {
        SCOPED_TRACE(Core::toString(endState));
        expectStartsNewGame(endState, Core::Command::Confirm);
    }
}

TEST(GameScreenTest, EndScreenResetStartsNewGame)
{
    for (const Core::GameState endState : c_endStates)
    {
        SCOPED_TRACE(Core::toString(endState));
        expectStartsNewGame(endState, Core::Command::Reset);
    }
}

TEST(GameScreenTest, EndScreenQuitReturnsToMainMenu)
{
    for (const Core::GameState endState : c_endStates)
    {
        SCOPED_TRACE(Core::toString(endState));
        expectScreenChange(endState, Core::Command::Quit, Core::GameState::MainMenu);
    }
}

// --- Screen state machine: every "-" cell of the transition table ---

namespace
{

struct IgnoredCommandCase
{
    Core::GameState state{Core::GameState::MainMenu};
    Core::Command command{Core::Command::None};
};

// gtest prints unknown parameters as raw bytes, so a failure names the table cell instead
void PrintTo(const IgnoredCommandCase& cell, std::ostream* out)
{
    *out << Core::toString(cell.state) << " + " << Core::toString(cell.command);
}

std::string ignoredCaseName(const testing::TestParamInfo<IgnoredCommandCase>& info)
{
    return std::format("{}_{}", Core::toString(info.param.state), Core::toString(info.param.command));
}

constexpr std::array c_ignoredCommandCases{
    IgnoredCommandCase{.state = Core::GameState::MainMenu, .command = Core::Command::TogglePause},
    IgnoredCommandCase{.state = Core::GameState::MainMenu, .command = Core::Command::Pause},
    IgnoredCommandCase{.state = Core::GameState::MainMenu, .command = Core::Command::Resume},
    IgnoredCommandCase{.state = Core::GameState::MainMenu, .command = Core::Command::Reset},
    IgnoredCommandCase{.state = Core::GameState::Playing, .command = Core::Command::Confirm},
    IgnoredCommandCase{.state = Core::GameState::Playing, .command = Core::Command::Resume},
    IgnoredCommandCase{.state = Core::GameState::Paused, .command = Core::Command::Confirm},
    IgnoredCommandCase{.state = Core::GameState::Paused, .command = Core::Command::Pause},
    IgnoredCommandCase{.state = Core::GameState::Victory, .command = Core::Command::TogglePause},
    IgnoredCommandCase{.state = Core::GameState::Victory, .command = Core::Command::Pause},
    IgnoredCommandCase{.state = Core::GameState::Victory, .command = Core::Command::Resume},
    IgnoredCommandCase{.state = Core::GameState::GameOver, .command = Core::Command::TogglePause},
    IgnoredCommandCase{.state = Core::GameState::GameOver, .command = Core::Command::Pause},
    IgnoredCommandCase{.state = Core::GameState::GameOver, .command = Core::Command::Resume},
    IgnoredCommandCase{.state = Core::GameState::Exit, .command = Core::Command::Confirm},
    IgnoredCommandCase{.state = Core::GameState::Exit, .command = Core::Command::TogglePause},
    IgnoredCommandCase{.state = Core::GameState::Exit, .command = Core::Command::Pause},
    IgnoredCommandCase{.state = Core::GameState::Exit, .command = Core::Command::Resume},
    IgnoredCommandCase{.state = Core::GameState::Exit, .command = Core::Command::Reset},
    IgnoredCommandCase{.state = Core::GameState::Exit, .command = Core::Command::Quit},
};

class GameIgnoredCommandTest : public testing::TestWithParam<IgnoredCommandCase>
{
};

}  // namespace

TEST_P(GameIgnoredCommandTest, CommandChangesNothing)
{
    const IgnoredCommandCase& cell{GetParam()};
    Core::Game game{makeGameIn(cell.state)};
    ASSERT_EQ(game.state(), cell.state);
    const Core::GameSnapshot before{game.snapshot()};

    game.handleCommand(cell.command);
    const Core::GameSnapshot after{game.snapshot()};
    EXPECT_EQ(after.state, cell.state);
    expectSameProgress(before, after);
    expectEntitiesFrozen(before, after);
}

INSTANTIATE_TEST_SUITE_P(ScreenTable, GameIgnoredCommandTest, testing::ValuesIn(c_ignoredCommandCases), ignoredCaseName);

// --- Domain events: the queue the presentation drains once per frame ---

namespace
{

using Events = std::vector<Core::GameEvent>;

// On c_collisionLevel the number of pellets eaten before the collision depends on timing, so a round there is
// checked by its edges: it starts with RoundStarted and ends with the one and only PlayerLostLife, and no GameOver yet
void expectRoundEndsWithLostLife(const Events& events)
{
    ASSERT_FALSE(events.empty());
    EXPECT_EQ(events.front(), Core::GameEvent::RoundStarted);
    EXPECT_EQ(events.back(), Core::GameEvent::PlayerLostLife);
    EXPECT_EQ(std::ranges::count(events, Core::GameEvent::PlayerLostLife), 1);
    EXPECT_EQ(std::ranges::count(events, Core::GameEvent::GameOver), 0);
}

}  // namespace

TEST(GameEventTest, GameEventToString)
{
    EXPECT_EQ(Core::toString(Core::GameEvent::PelletEaten), "PelletEaten");
    EXPECT_EQ(Core::toString(Core::GameEvent::PowerPelletEaten), "PowerPelletEaten");
    EXPECT_EQ(Core::toString(Core::GameEvent::GhostEaten), "GhostEaten");
    EXPECT_EQ(Core::toString(Core::GameEvent::PlayerLostLife), "PlayerLostLife");
    EXPECT_EQ(Core::toString(Core::GameEvent::RoundStarted), "RoundStarted");
    EXPECT_EQ(Core::toString(Core::GameEvent::Victory), "Victory");
    EXPECT_EQ(Core::toString(Core::GameEvent::GameOver), "GameOver");
}

TEST(GameEventTest, RoundStartedOnReadyToActiveAndTakeClearsQueue)
{
    Core::Game game{Tests::makeReadyGame()};
    // Confirm went through reset(), so the Ready pause starts with an empty queue
    EXPECT_TRUE(game.takeEvents().empty());

    skipReadyPhase(game);
    ASSERT_EQ(game.snapshot().roundPhase, Core::RoundPhase::Active);
    EXPECT_EQ(game.takeEvents(), (Events{Core::GameEvent::RoundStarted}));
    EXPECT_TRUE(game.takeEvents().empty());
}

// The main loop runs several fixed steps per frame and drains once, so nothing may be lost between updates
TEST(GameEventTest, EventsOfConsecutiveUpdatesAccumulate)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(1.0f));
    game.update(secondsFor(1.0f));

    EXPECT_EQ(game.takeEvents(), (Events{Core::GameEvent::PelletEaten, Core::GameEvent::PelletEaten}));
}

TEST(GameEventTest, PelletEaten)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(1.0f));

    EXPECT_EQ(game.takeEvents(), (Events{Core::GameEvent::PelletEaten}));
}

TEST(GameEventTest, PowerPelletEaten)
{
    Core::Game game{makeFixedRandomGame(c_frightenedRingLevel)};
    game.handleCommand(Core::Command::MoveRight);
    Core::GameSnapshot before{};
    ASSERT_TRUE(stepUntil(game, isFrightenedActive, before));

    EXPECT_EQ(game.takeEvents(), (Events{Core::GameEvent::PowerPelletEaten}));
}

TEST(GameEventTest, VictoryAfterLastPellet)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(3.0f));
    ASSERT_EQ(game.state(), Core::GameState::Victory);

    EXPECT_EQ(game.takeEvents(),
        (Events{Core::GameEvent::PelletEaten, Core::GameEvent::PelletEaten, Core::GameEvent::PowerPelletEaten, Core::GameEvent::Victory}));
}

TEST(GameEventTest, GhostEaten)
{
    Core::Game game{makeFixedRandomGame(c_eatGhostLevel)};
    Core::GameSnapshot before{};
    ASSERT_TRUE(stepUntil(
        game, [](const Core::GameSnapshot& snapshot) { return ghostOf(snapshot, Core::GhostId::Ruby).position.x < 5.0f; }, before));
    game.handleCommand(Core::Command::MoveRight);
    ASSERT_TRUE(stepUntil(game, isRubyEaten, before));

    EXPECT_EQ(game.takeEvents(), (Events{Core::GameEvent::PowerPelletEaten, Core::GameEvent::GhostEaten}));
}

TEST(GameEventTest, PlayerLostLifeImmediatelyOnCollision)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(runIntoGhost(game));
    ASSERT_EQ(game.snapshot().roundPhase, Core::RoundPhase::PlayerDying);

    // runIntoGhost stops on the collision substep: the death sound must not wait for the dying pause
    expectRoundEndsWithLostLife(game.takeEvents());

    game.update(0.5f);
    EXPECT_EQ(game.snapshot().roundPhase, Core::RoundPhase::PlayerDying);
    EXPECT_TRUE(game.takeEvents().empty());
}

TEST(GameEventTest, GameOverAfterDyingTimer)
{
    Core::Game game{Tests::makeReadyGame(c_collisionLevel)};
    ASSERT_TRUE(loseLife(game));
    ASSERT_TRUE(loseLife(game));
    (void)game.takeEvents();

    // The last collision is reported at once, but GameOver waits for the dying pause
    ASSERT_TRUE(runIntoGhost(game));
    ASSERT_EQ(game.snapshot().lives, 0);
    expectRoundEndsWithLostLife(game.takeEvents());

    ASSERT_TRUE(runWhilePhase(game, Core::RoundPhase::PlayerDying));
    ASSERT_EQ(game.state(), Core::GameState::GameOver);
    EXPECT_EQ(game.takeEvents(), (Events{Core::GameEvent::GameOver}));
}

// Sounds of an abandoned game must not leak into the new one
TEST(GameEventTest, ResetClearsUntakenEvents)
{
    Core::Game game{makeCorridorGame()};
    game.update(secondsFor(1.0f));
    ASSERT_EQ(game.snapshot().score, Core::c_pelletScore);

    game.handleCommand(Core::Command::Reset);
    EXPECT_TRUE(game.takeEvents().empty());
}
