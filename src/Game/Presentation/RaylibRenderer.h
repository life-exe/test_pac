#pragma once

#include "Game.h"

#include <string>

namespace LifeExe::Presentation
{

class RaylibRenderer
{
public:
    RaylibRenderer() = default;
    ~RaylibRenderer();

    // Prevent accidental copying of window handle / graphics context
    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;

    void init(int width, int height, const std::string& title);
    void render(const Core::Game& game) const;
    void processInput(Core::Game& game) const;
    void close();
    bool shouldClose() const;

private:
    bool m_initialized{false};
};

}  // namespace LifeExe::Presentation