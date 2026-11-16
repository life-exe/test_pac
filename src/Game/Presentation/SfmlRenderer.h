#pragma once

#include "Command.h"
#include "GameSnapshot.h"

#include <memory>
#include <string>
#include <vector>

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

    SfmlRenderer(const SfmlRenderer&) = delete;
    SfmlRenderer& operator=(const SfmlRenderer&) = delete;

    bool init(int width, int height, const std::string& title);
    void close();

    std::vector<Core::Command> processInput();
    [[nodiscard]] bool shouldClose() const;

    void render(const Core::GameSnapshot& snapshot);

private:
    std::unique_ptr<sf::RenderWindow> m_window;
    bool m_shouldClose{false};
};

}  // namespace LifeExe::Presentation
