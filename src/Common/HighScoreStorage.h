#pragma once

#include <filesystem>

namespace LifeExe
{

/// Reads the saved high score from a JSON file.
///
/// Expected document: `{"highScore": N}` with a non-negative integer N.
/// A missing record is a normal first launch, so the function never throws.
///
/// @param path file with the saved record, for example highscore.json.
/// @return the saved record; 0 if the file is missing, empty or not valid JSON,
///         the key is missing, the value is not an integer or is negative.
int loadHighScore(const std::filesystem::path& path);

/// Writes the high score to a JSON file as `{"highScore": N}`, replacing its content.
///
/// A record that cannot be written must not stop the game, so a file that cannot be
/// opened is skipped silently and the function never throws.
///
/// @param path file to write, for example highscore.json.
/// @param highScore record to save; negative values are saved as 0.
void saveHighScore(const std::filesystem::path& path, int highScore);

}  // namespace LifeExe
