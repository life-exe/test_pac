#pragma once

namespace LifeExe::Core
{

inline constexpr float c_playerSpeed{8.0f};         // tiles per second
inline constexpr float c_maxSubstep{1.0f / 64.0f};  // seconds (exact power of two in IEEE-754)
inline constexpr int c_pelletScore{10};
inline constexpr int c_powerPelletScore{50};

}  // namespace LifeExe::Core
