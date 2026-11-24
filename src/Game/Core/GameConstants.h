#pragma once

namespace LifeExe::Core
{

inline constexpr float c_playerSpeed{8.0f};         // tiles per second
inline constexpr float c_ghostSpeed{5.0f};          // tiles per second
inline constexpr float c_maxSubstep{1.0f / 64.0f};  // seconds (exact power of two in IEEE-754)
inline constexpr int c_pelletScore{10};
inline constexpr int c_powerPelletScore{50};
inline constexpr float c_rosieReleaseDelay{3.0f};   // seconds
inline constexpr float c_indigoReleaseDelay{6.0f};  // seconds
inline constexpr float c_coralReleaseDelay{9.0f};   // seconds
inline constexpr float c_scatterDuration{7.0f};     // seconds
inline constexpr float c_chaseDuration{20.0f};      // seconds
inline constexpr int c_startingLives{3};
inline constexpr float c_readyDuration{1.5f};  // seconds
inline constexpr float c_dyingDuration{1.0f};  // seconds
inline constexpr float c_frightenedSpeedMultiplier{0.5f};
inline constexpr float c_eatenSpeedMultiplier{2.0f};
inline constexpr float c_frightenedDuration{7.0f};   // seconds
inline constexpr float c_frightenedFlashTime{2.0f};  // seconds before frightened ends
inline constexpr int c_baseGhostEatScore{200};

}  // namespace LifeExe::Core
