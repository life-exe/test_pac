#pragma once

#include "GameEvents.h"

#include <raylib.h>

#include <vector>

namespace LifeExe::Presentation
{

// Owns the raylib audio device and the effect sounds for its whole lifetime.
// Without an audio device (WSL, CI) every call is a silent no-op, so the game runs the same with or without sound.
class SoundSystem
{
public:
    SoundSystem();
    ~SoundSystem();

    SoundSystem(const SoundSystem&) = delete;
    SoundSystem& operator=(const SoundSystem&) = delete;

    void processEvents(const std::vector<Core::GameEvent>& events);

private:
    void playEvent(Core::GameEvent event);

    bool m_ready{false};
    Sound m_chomp{};
    Sound m_powerPellet{};
    Sound m_eatGhost{};
    Sound m_death{};
    Sound m_victory{};
};

}  // namespace LifeExe::Presentation
