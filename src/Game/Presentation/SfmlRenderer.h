#pragma once

#include "Game.h"

#include <string>
#include <memory>

namespace sf
{
class RenderWindow;
}

namespace LifeExe::Presentation
{

class SfmlRenderer
{
public:
    SfmlRenderer();
    ~SfmlRenderer();

    // Prevent accidental copying of window handle / graphics context
    SfmlRenderer(const SfmlRenderer&) = delete;
    SfmlRenderer& operator=(const SfmlRenderer&) = delete;

    bool init(int width, int height, const std::string& title);
    void close();
    [[nodiscard]] bool shouldClose();
    void render(const Core::Game& game) const;
    void processInput(Core::Game& game);

private:
    std::unique_ptr<sf::RenderWindow> m_window;
    bool m_shouldClose{false};
};

}  // namespace LifeExe::Presentation