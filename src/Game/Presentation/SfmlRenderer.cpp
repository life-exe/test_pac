#include "SfmlRenderer.h"

#include <SFML/Graphics.hpp>
#include <optional>

namespace LifeExe::Presentation
{
SfmlRenderer::SfmlRenderer() = default;

SfmlRenderer::~SfmlRenderer()
{
    close();
}

bool SfmlRenderer::init(int width, int height, const std::string& title)
{
    if (m_window && m_window->isOpen())
    {
        return true;
    }

    m_window =
        std::make_unique<sf::RenderWindow>(sf::VideoMode({static_cast<unsigned int>(width), static_cast<unsigned int>(height)}), title);
    m_window->setFramerateLimit(60);
    m_shouldClose = false;
    return true;
}

void SfmlRenderer::render([[maybe_unused]] const Core::Game& game) const
{
    if (!m_window || !m_window->isOpen())
    {
        return;
    }

    // 1. Clear background
    m_window->clear(sf::Color::Blue);

    // 2. Query window size
    const auto size = m_window->getSize();
    const float centerX = static_cast<float>(size.x) / 2.0f;
    const float centerY = static_cast<float>(size.y) / 2.0f;

    // 3. Draw state indicator
    sf::RectangleShape rect({100.0f, 100.0f});
    rect.setOrigin({50.0f, 50.0f});
    rect.setPosition({centerX, centerY});

    switch (game.state())
    {
        case Core::GameState::Playing:
            rect.setFillColor(sf::Color(46, 204, 113));  // Green
            break;
        case Core::GameState::Paused:
            rect.setFillColor(sf::Color(243, 156, 18));  // Orange
            break;
        case Core::GameState::GameOver:
            rect.setFillColor(sf::Color(231, 76, 60));  // Red
            break;
    }

    m_window->draw(rect);

    // 4. Present frame
    m_window->display();
}

void SfmlRenderer::close()
{
    if (m_window && m_window->isOpen())
    {
        m_window->close();
    }
    m_window.reset();
}

bool SfmlRenderer::shouldClose()
{
    return m_shouldClose || (!m_window || !m_window->isOpen());
}

void SfmlRenderer::processInput(Core::Game& game)
{
    if (!m_window)
    {
        return;
    }

    while (const std::optional event = m_window->pollEvent())
    {
        // 1. Проверка закрытия окна (крестик)
        if (event->is<sf::Event::Closed>())
        {
            m_shouldClose = true;
            game.handleCommand(Core::Command::Quit);
        }
        // 2. Проверка нажатия клавиш
        else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            switch (keyPressed->code)
            {
                case sf::Keyboard::Key::P:
                case sf::Keyboard::Key::Space:
                    if (game.state() == Core::GameState::Playing)
                    {
                        game.handleCommand(Core::Command::Pause);
                    }
                    else if (game.state() == Core::GameState::Paused)
                    {
                        game.handleCommand(Core::Command::Resume);
                    }
                    break;

                case sf::Keyboard::Key::R: game.handleCommand(Core::Command::Reset); break;

                case sf::Keyboard::Key::Escape:
                    m_shouldClose = true;
                    game.handleCommand(Core::Command::Quit);
                    break;

                default: break;
            }
        }
    }
}

}  // namespace LifeExe::Presentation