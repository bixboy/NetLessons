#include "MiniGames/MiniGameJustePrix.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include <cmath>
#include <string>

void MiniGameJustePrix::Draw(ClientContext& m_ctx, UIRenderer& m_ui)
{
    float winW = static_cast<float>(m_ctx.WindowSize.x);
    float winH = static_cast<float>(m_ctx.WindowSize.y);
    float centerY = winH / 2.f;
    float offset = centerY - 230.f;

    m_ui.DrawPanel(m_ctx.CenterX - 200.f, 10.f + offset, 400.f, 50.f, sf::Color(15, 15, 25, 230));

    sf::Text title(m_ctx.Font);
    title.setString("TROUVE LE NOMBRE");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 20.f + offset, 22);
    m_ctx.Target->draw(title);

    // Hint feedback
    if (m_ctx.LastHint != 0)
    {
        float hintY = 75.f + offset;
        bool isPlus = (m_ctx.LastHint == 1);
        sf::Color hintColor = isPlus ? sf::Color(100, 255, 150) : sf::Color(255, 100, 100);

        float bounce = std::sin(m_ctx.AnimClock.getElapsedTime().asSeconds() * 6.f) * 8.f;

        sf::Text arrow(m_ctx.Font);
        arrow.setString(isPlus ? "+" : "-");
        arrow.setCharacterSize(50);
        arrow.setFillColor(hintColor);
        arrow.setStyle(sf::Text::Bold);
        m_ui.CenterText(arrow, hintY + (isPlus ? -bounce : bounce), 50);
        m_ctx.Target->draw(arrow);

        sf::Text hintText(m_ctx.Font);
        hintText.setString(isPlus ? "C'est PLUS !" : "C'est MOINS !");
        hintText.setFillColor(hintColor);
        hintText.setStyle(sf::Text::Bold);
        m_ui.CenterText(hintText, hintY + 55.f, 18);
        m_ctx.Target->draw(hintText);
    }
    else
    {
        sf::Text msg(m_ctx.Font);
        msg.setString(m_ctx.ServerMessage);
        msg.setFillColor(m_ctx.MessageColor);
        m_ui.CenterText(msg, 90.f + offset, 16);
        m_ctx.Target->draw(msg);
    }

    // "+" panel (left)
    float btnW = 120.f, btnH = 120.f;
    float leftX = 40.f;
    float btnY = 180.f + offset;

    sf::RectangleShape plusPanel({btnW, btnH});
    plusPanel.setPosition({leftX, btnY});
    plusPanel.setFillColor(sf::Color(20, 50, 30, 220));
    plusPanel.setOutlineThickness(3.f);
    uint8_t pGreen = static_cast<uint8_t>(180 + m_ctx.PulseValue * 75);
    plusPanel.setOutlineColor(sf::Color(50, pGreen, 80));
    m_ctx.Target->draw(plusPanel);

    sf::Text plusText(m_ctx.Font);
    plusText.setString("+");
    plusText.setCharacterSize(60);
    plusText.setFillColor(sf::Color(100, 255, 150));
    plusText.setStyle(sf::Text::Bold);
    sf::FloatRect pBounds = plusText.getLocalBounds();
    plusText.setPosition({leftX + btnW / 2.f - pBounds.size.x / 2.f, btnY + btnH / 2.f - pBounds.size.y});
    m_ctx.Target->draw(plusText);

    // "-" panel (right)
    float rightX = winW - 40.f - btnW;

    sf::RectangleShape minusPanel({btnW, btnH});
    minusPanel.setPosition({rightX, btnY});
    minusPanel.setFillColor(sf::Color(50, 20, 20, 220));
    minusPanel.setOutlineThickness(3.f);
    uint8_t pRed = static_cast<uint8_t>(180 + m_ctx.PulseValue * 75);
    minusPanel.setOutlineColor(sf::Color(pRed, 50, 50));
    m_ctx.Target->draw(minusPanel);

    sf::Text minusText(m_ctx.Font);
    minusText.setString("-");
    minusText.setCharacterSize(60);
    minusText.setFillColor(sf::Color(255, 100, 100));
    minusText.setStyle(sf::Text::Bold);
    sf::FloatRect mBounds = minusText.getLocalBounds();
    minusText.setPosition({rightX + btnW / 2.f - mBounds.size.x / 2.f, btnY + btnH / 2.f - mBounds.size.y});
    m_ctx.Target->draw(minusText);

    // Number circle
    float circleRadius = 80.f;
    float circleY = 240.f + offset;

    sf::CircleShape circle(circleRadius);
    circle.setFillColor(sf::Color(20, 20, 35));
    circle.setOutlineThickness(4.f);

    sf::Color outlineColor;
    if (m_ctx.LastHint == 1)
    {
        uint8_t p = static_cast<uint8_t>(m_ctx.PulseValue * 50);
        outlineColor = sf::Color(50 + p, 220 + p / 2, 100 + p);
    }
    else if (m_ctx.LastHint == 2)
    {
        uint8_t p = static_cast<uint8_t>(m_ctx.PulseValue * 50);
        outlineColor = sf::Color(220 + p / 2, 50 + p, 50 + p);
    }
    else
    {
        uint8_t p = static_cast<uint8_t>(m_ctx.PulseValue * 50);
        outlineColor = sf::Color(0, 180 + p, 230 + p / 2);
    }
    
    circle.setOutlineColor(outlineColor);
    circle.setOrigin({circleRadius, circleRadius});
    circle.setPosition({m_ctx.CenterX, circleY});
    m_ctx.Target->draw(circle);

    sf::Text nb(m_ctx.Font);
    nb.setString(std::to_string(m_ctx.CurrentNumberChoice));
    nb.setFillColor(sf::Color::White);
    nb.setStyle(sf::Text::Bold);
    nb.setCharacterSize(55);
    sf::FloatRect nbBounds = nb.getLocalBounds();
    nb.setPosition({m_ctx.CenterX - nbBounds.size.x / 2.f, circleY - nbBounds.size.y});
    m_ctx.Target->draw(nb);

    if (m_ctx.GuessAttempts > 0)
    {
        sf::Text attempts(m_ctx.Font);
        attempts.setString("Tentative #" + std::to_string(m_ctx.GuessAttempts));
        attempts.setFillColor(sf::Color(150, 150, 180));
        m_ui.CenterText(attempts, circleY + circleRadius + 15.f, 14);
        m_ctx.Target->draw(attempts);
    }

    // Valider button
    float validerY = 400.f + offset;
    m_ui.DrawButton("VALIDER", validerY, true);

    sf::Text hint(m_ctx.Font);
    hint.setString("Deplacez-vous vers les boutons et appuyez [E]");
    hint.setFillColor(sf::Color(80, 80, 110));
    m_ui.CenterText(hint, validerY + 40.f, 12);
    m_ctx.Target->draw(hint);
}
