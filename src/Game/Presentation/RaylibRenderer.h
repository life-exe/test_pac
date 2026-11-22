#pragma once

#include "GameSnapshot.h"
#include "Command.h"

#include <raylib.h>

#include <string>
#include <vector>

namespace LifeExe::Presentation
{

class RaylibRenderer
{
public:
    RaylibRenderer() = default;
    ~RaylibRenderer();

    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;

    void init(int width, int height, const std::string& title);
    void close();
    [[nodiscard]] bool shouldClose() const;

    void render(const Core::GameSnapshot& snapshot) const;
    std::vector<Core::Command> processInput() const;

private:
    bool m_initialized{false};
    Texture2D m_spritesheet{};
};

}  // namespace LifeExe::Presentation
