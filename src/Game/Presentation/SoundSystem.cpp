#include "SoundSystem.h"

#include <raylib.h>

#include <vector>

namespace LifeExe::Presentation
{

namespace
{

// A missing WAV loads as an empty Sound; skipping it keeps a lost asset silent instead of fatal
void playIfValid(const Sound& sound)
{
    if (IsSoundValid(sound))
    {
        PlaySound(sound);
    }
}

void unloadIfValid(const Sound& sound)
{
    if (IsSoundValid(sound))
    {
        UnloadSound(sound);
    }
}

}  // namespace

SoundSystem::SoundSystem()
{
    InitAudioDevice();
    m_ready = IsAudioDeviceReady();
    if (!m_ready)
    {
        return;
    }

    // Paths are relative to the working directory, like assets/spritesheet.png in RaylibRenderer
    m_chomp = LoadSound("assets/sounds/chomp.wav");
    m_powerPellet = LoadSound("assets/sounds/power_pellet.wav");
    m_eatGhost = LoadSound("assets/sounds/eat_ghost.wav");
    m_death = LoadSound("assets/sounds/death.wav");
    m_victory = LoadSound("assets/sounds/victory.wav");
}

SoundSystem::~SoundSystem()
{
    if (!m_ready)
    {
        return;
    }

    // Sound buffers belong to the audio context, so they are freed before the device closes
    unloadIfValid(m_chomp);
    unloadIfValid(m_powerPellet);
    unloadIfValid(m_eatGhost);
    unloadIfValid(m_death);
    unloadIfValid(m_victory);
    CloseAudioDevice();
}

void SoundSystem::processEvents(const std::vector<Core::GameEvent>& events)
{
    if (!m_ready)
    {
        return;
    }

    for (const Core::GameEvent event : events)
    {
        playEvent(event);
    }
}

void SoundSystem::playEvent(Core::GameEvent event)
{
    switch (event)
    {
        case Core::GameEvent::PelletEaten:
            // Pellets come several times a second; restarting the chomp each time would crackle
            if (!IsSoundPlaying(m_chomp))
            {
                playIfValid(m_chomp);
            }
            break;
        case Core::GameEvent::PowerPelletEaten: playIfValid(m_powerPellet); break;
        case Core::GameEvent::GhostEaten: playIfValid(m_eatGhost); break;
        case Core::GameEvent::PlayerLostLife: playIfValid(m_death); break;
        case Core::GameEvent::Victory: playIfValid(m_victory); break;
        // The death sound already plays on PlayerLostLife, and a new round needs no cue
        case Core::GameEvent::GameOver:
        case Core::GameEvent::RoundStarted: break;
    }
}

}  // namespace LifeExe::Presentation
