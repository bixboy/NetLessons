#include "UI/UIRenderer.h"
#include "ClientContext.h"
#include <cmath>


UIRenderer::UIRenderer(ClientContext& ctx)
    : m_ctx(ctx)
{
}

void UIRenderer::DrawBackground()
{
    sf::Color lineColor(30, 30, 45, 100);
    for (int i = 0; i < 20; i++)
    {
        sf::RectangleShape line({static_cast<float>(m_ctx.WindowSize.x), 1.f});
        line.setPosition({0, static_cast<float>(i * 40)});
        line.setFillColor(lineColor);
        m_ctx.Window.draw(line);
    }
}

void UIRenderer::DrawPanel(float x, float y, float w, float h, sf::Color color, float cornerRadius)
{
    sf::RectangleShape panel({w, h});
    panel.setPosition({x, y});
    panel.setFillColor(color);
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(60, 60, 80, 200));
    m_ctx.Window.draw(panel);
}

void UIRenderer::DrawButton(const std::string& textStr, float y, bool active, bool hovered)
{
    float btnWidth = 320.f;
    float btnHeight = 45.f;

    sf::RectangleShape btn({btnWidth, btnHeight});
    btn.setOrigin({btnWidth / 2.f, btnHeight / 2.f});
    btn.setPosition({m_ctx.CenterX, y});

    sf::Color baseColor = active ? sf::Color(0, 180, 230) : sf::Color(50, 50, 60);
    sf::Color borderColor = active ? sf::Color(0, 220, 255) : sf::Color(80, 80, 100);

    if (active)
    {
        uint8_t pulse = static_cast<uint8_t>(m_ctx.PulseValue * 30);
        baseColor = sf::Color(0, 180 + pulse, 230 + pulse / 2);
    }

    btn.setFillColor(baseColor);
    btn.setOutlineThickness(2.f);
    btn.setOutlineColor(borderColor);

    m_ctx.Window.draw(btn);

    sf::Text text(m_ctx.Font);
    text.setString(textStr);
    text.setCharacterSize(16);
    text.setFillColor(active ? sf::Color::White : sf::Color(180, 180, 180));
    text.setStyle(sf::Text::Bold);
    CenterText(text, y - 10.f, 16);
    m_ctx.Window.draw(text);
}

void UIRenderer::DrawInputBox(const std::string& label, const std::string& value, float y, bool active)
{
    sf::Text lbl(m_ctx.Font);
    lbl.setString(label);
    lbl.setFillColor(sf::Color(150, 150, 180));
    lbl.setCharacterSize(14);
    CenterText(lbl, y - 45.f, 14);
    m_ctx.Window.draw(lbl);

    float boxWidth = 320.f;
    float boxHeight = 45.f;

    sf::RectangleShape box({boxWidth, boxHeight});
    box.setOrigin({boxWidth / 2.f, boxHeight / 2.f});
    box.setPosition({m_ctx.CenterX, y});
    box.setFillColor(sf::Color(25, 25, 35));
    box.setOutlineThickness(active ? 2.f : 1.f);
    box.setOutlineColor(active ? sf::Color(0, 200, 255) : sf::Color(80, 80, 100));
    m_ctx.Window.draw(box);

    std::string displayValue = value;
    if (active)
    {
        float blinkTime = m_ctx.AnimClock.getElapsedTime().asSeconds();
        if (fmod(blinkTime, 1.0f) < 0.5f)
        {
            displayValue += "|";
        }
    }

    sf::Text val(m_ctx.Font);
    val.setString(displayValue);
    val.setFillColor(sf::Color::White);
    val.setCharacterSize(18);
    CenterText(val, y - 10.f, 18);
    m_ctx.Window.draw(val);
}

void UIRenderer::CenterText(sf::Text& text, float y, int fontSize)
{
    text.setCharacterSize(fontSize);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition({m_ctx.CenterX - (bounds.size.x / 2.f), y});
}
