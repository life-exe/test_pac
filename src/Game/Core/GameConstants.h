#pragma once

namespace LifeExe::Core
{

inline constexpr float c_playerSpeed{8.0f};         // tiles per second
inline constexpr float c_ghostSpeed{7.5f};          // tiles per second
inline constexpr float c_maxSubstep{1.0f / 64.0f};  // seconds (exact power of two in IEEE-754)
inline constexpr int c_pelletScore{10};
inline constexpr int c_powerPelletScore{50};
inline constexpr float c_rosieReleaseDelay{3.0f};   // seconds before Rosie leaves the ghost house
inline constexpr float c_indigoReleaseDelay{6.0f};  // seconds before Indigo leaves the ghost house
inline constexpr float c_coralReleaseDelay{9.0f};   // seconds before Coral leaves the ghost house

}  // namespace LifeExe::Core
