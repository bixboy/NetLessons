#include "Screens/ScreenGame.h"
#include "Screens/ScreenIpConfig.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include <cmath>
#include <algorithm>


ScreenGame::ScreenGame(ClientContext& ctx, UIRenderer& ui)
    : m_ctx(ctx), m_ui(ui)
{
}

void ScreenGame::HandleInput(const sf::Event& event)
{
    // Input is handled by AvatarManager interaction zones
}

void ScreenGame::Draw()
{
    if (m_ctx.ActiveGameID == 0)
    {
        DrawJustePrix();
    }
    else if (m_ctx.ActiveGameID == 1)
    {
        DrawHotPotato();
    }

    // Spectator Overlay
    if (m_ctx.IsSpectator())
    {
        sf::Text spec(m_ctx.Font);
        spec.setString("--- MODE SPECTATEUR ---");
        spec.setCharacterSize(24);
        spec.setFillColor(sf::Color(200, 200, 200, 180));
        spec.setStyle(sf::Text::Bold);
        
        sf::FloatRect b = spec.getLocalBounds();
        spec.setPosition({m_ctx.CenterX - b.size.x / 2.f, 20.f});
        
        sf::RectangleShape bg({b.size.x + 40.f, b.size.y + 20.f});
        bg.setPosition({m_ctx.CenterX - b.size.x / 2.f - 20.f, 15.f});
        bg.setFillColor(sf::Color(0, 0, 0, 150));
        
        m_ctx.Window.draw(bg);
        m_ctx.Window.draw(spec);
    }
}

// ============================================================
// JUSTE PRIX UI
// ============================================================
void ScreenGame::DrawJustePrix()
{
    float winW = static_cast<float>(m_ctx.WindowSize.x);
    float winH = static_cast<float>(m_ctx.WindowSize.y);
    float centerY = winH / 2.f;
    float offset = centerY - 230.f;

    // Title panel
    m_ui.DrawPanel(m_ctx.CenterX - 200.f, 10.f + offset, 400.f, 50.f, sf::Color(15, 15, 25, 230));

    sf::Text title(m_ctx.Font);
    title.setString("TROUVE LE NOMBRE");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 20.f + offset, 22);
    m_ctx.Window.draw(title);

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
        m_ctx.Window.draw(arrow);

        sf::Text hintText(m_ctx.Font);
        hintText.setString(isPlus ? "C'est PLUS !" : "C'est MOINS !");
        hintText.setFillColor(hintColor);
        hintText.setStyle(sf::Text::Bold);
        m_ui.CenterText(hintText, hintY + 55.f, 18);
        m_ctx.Window.draw(hintText);
    }
    else
    {
        sf::Text msg(m_ctx.Font);
        msg.setString(m_ctx.ServerMessage);
        msg.setFillColor(m_ctx.MessageColor);
        m_ui.CenterText(msg, 90.f + offset, 16);
        m_ctx.Window.draw(msg);
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
    m_ctx.Window.draw(plusPanel);

    sf::Text plusText(m_ctx.Font);
    plusText.setString("+");
    plusText.setCharacterSize(60);
    plusText.setFillColor(sf::Color(100, 255, 150));
    plusText.setStyle(sf::Text::Bold);
    sf::FloatRect pBounds = plusText.getLocalBounds();
    plusText.setPosition({leftX + btnW / 2.f - pBounds.size.x / 2.f, btnY + btnH / 2.f - pBounds.size.y});
    m_ctx.Window.draw(plusText);

    // "-" panel (right)
    float rightX = winW - 40.f - btnW;

    sf::RectangleShape minusPanel({btnW, btnH});
    minusPanel.setPosition({rightX, btnY});
    minusPanel.setFillColor(sf::Color(50, 20, 20, 220));
    minusPanel.setOutlineThickness(3.f);
    uint8_t pRed = static_cast<uint8_t>(180 + m_ctx.PulseValue * 75);
    minusPanel.setOutlineColor(sf::Color(pRed, 50, 50));
    m_ctx.Window.draw(minusPanel);

    sf::Text minusText(m_ctx.Font);
    minusText.setString("-");
    minusText.setCharacterSize(60);
    minusText.setFillColor(sf::Color(255, 100, 100));
    minusText.setStyle(sf::Text::Bold);
    sf::FloatRect mBounds = minusText.getLocalBounds();
    minusText.setPosition({rightX + btnW / 2.f - mBounds.size.x / 2.f, btnY + btnH / 2.f - mBounds.size.y});
    m_ctx.Window.draw(minusText);

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
    m_ctx.Window.draw(circle);

    sf::Text nb(m_ctx.Font);
    nb.setString(std::to_string(m_ctx.CurrentNumberChoice));
    nb.setFillColor(sf::Color::White);
    nb.setStyle(sf::Text::Bold);
    nb.setCharacterSize(55);
    sf::FloatRect nbBounds = nb.getLocalBounds();
    nb.setPosition({m_ctx.CenterX - nbBounds.size.x / 2.f, circleY - nbBounds.size.y});
    m_ctx.Window.draw(nb);

    // Attempt counter
    if (m_ctx.GuessAttempts > 0)
    {
        sf::Text attempts(m_ctx.Font);
        attempts.setString("Tentative #" + std::to_string(m_ctx.GuessAttempts));
        attempts.setFillColor(sf::Color(150, 150, 180));
        m_ui.CenterText(attempts, circleY + circleRadius + 15.f, 14);
        m_ctx.Window.draw(attempts);
    }

    // Valider button
    float validerY = 400.f + offset;
    m_ui.DrawButton("VALIDER", validerY, true);

    sf::Text hint(m_ctx.Font);
    hint.setString("Deplacez-vous vers les boutons et appuyez [E]");
    hint.setFillColor(sf::Color(80, 80, 110));
    m_ui.CenterText(hint, validerY + 40.f, 12);
    m_ctx.Window.draw(hint);
}

// ============================================================
// HOT POTATO UI
// ============================================================
void ScreenGame::DrawHotPotato()
{
    float winW = static_cast<float>(m_ctx.WindowSize.x);
    float winH = static_cast<float>(m_ctx.WindowSize.y);
    float centerY = winH / 2.f;
    float offset = centerY - 230.f;

    bool isBombHolder = (m_ctx.BombHolder == m_ctx.PseudoInput);
    float time = m_ctx.AnimClock.getElapsedTime().asSeconds();

    // ========== TITLE ==========
    m_ui.DrawPanel(m_ctx.CenterX - 200.f, 10.f + offset, 400.f, 50.f,
        isBombHolder ? sf::Color(60, 15, 15, 240) : sf::Color(15, 15, 25, 230));

    sf::Text title(m_ctx.Font);
    title.setString("BOMBE !");
    title.setFillColor(isBombHolder ? sf::Color(255, 80, 60) : sf::Color(255, 200, 50));
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 20.f + offset, 26);
    m_ctx.Window.draw(title);

    // ========== BIG TIMER ==========
    float timerY = 100.f + offset;
    float timerRadius = 90.f;

    // Timer ring (background)
    sf::CircleShape timerBg(timerRadius);
    timerBg.setOrigin({timerRadius, timerRadius});
    timerBg.setPosition({m_ctx.CenterX, timerY + timerRadius});
    timerBg.setFillColor(sf::Color(20, 20, 30));
    timerBg.setOutlineThickness(6.f);

    // Ring color: red pulse when < 3s, orange normal
    float timerVal = std::max(0.f, m_ctx.BombTimer);
    sf::Color ringColor;
    if (timerVal < 3.f)
    {
        float flash = std::sin(time * 12.f) * 0.5f + 0.5f;
        uint8_t r = static_cast<uint8_t>(200 + flash * 55);
        ringColor = sf::Color(r, 30, 30);
    }
    else
    {
        uint8_t p = static_cast<uint8_t>(m_ctx.PulseValue * 40);
        ringColor = sf::Color(255, 150 + p, 50);
    }
    timerBg.setOutlineColor(ringColor);
    m_ctx.Window.draw(timerBg);

    // Timer number
    sf::Text timerText(m_ctx.Font);
    int displaySeconds = static_cast<int>(std::ceil(timerVal));
    timerText.setString(std::to_string(displaySeconds));
    timerText.setCharacterSize(70);
    timerText.setFillColor(timerVal < 3.f ? sf::Color(255, 80, 60) : sf::Color::White);
    timerText.setStyle(sf::Text::Bold);
    sf::FloatRect tBounds = timerText.getLocalBounds();
    timerText.setPosition({m_ctx.CenterX - tBounds.size.x / 2.f, timerY + timerRadius - tBounds.size.y});
    m_ctx.Window.draw(timerText);

    // ========== BOMB HOLDER INFO ==========
    float infoY = timerY + timerRadius * 2.f + 20.f;

    sf::Text holderLabel(m_ctx.Font);
    if (isBombHolder)
    {
        holderLabel.setString("TU AS LA BOMBE !");
        holderLabel.setFillColor(sf::Color(255, 80, 60));
    }
    else
    {
        holderLabel.setString(m_ctx.BombHolder + " a la bombe");
        holderLabel.setFillColor(sf::Color(255, 200, 100));
    }
    holderLabel.setStyle(sf::Text::Bold);
    m_ui.CenterText(holderLabel, infoY, 20);
    m_ctx.Window.draw(holderLabel);

    // ========== STATUS MESSAGE ==========
    if (isBombHolder)
    {
        float bounce = std::sin(time * 4.f) * 3.f;
        sf::Text pushHint(m_ctx.Font);
        pushHint.setString("PUSH quelqu'un pour passer la bombe ! [SPACE]");
        pushHint.setFillColor(sf::Color(255, 150, 100));
        pushHint.setStyle(sf::Text::Bold);
        m_ui.CenterText(pushHint, infoY + 35.f + bounce, 14);
        m_ctx.Window.draw(pushHint);
    }
    else
    {
        sf::Text safeHint(m_ctx.Font);
        safeHint.setString("Evite le porteur de la bombe !");
        safeHint.setFillColor(sf::Color(100, 200, 100));
        m_ui.CenterText(safeHint, infoY + 35.f, 14);
        m_ctx.Window.draw(safeHint);
    }

    // ========== ALIVE PLAYERS ==========
    float listY = infoY + 70.f;
    sf::Text aliveLabel(m_ctx.Font);
    aliveLabel.setString("Survivants: " + std::to_string(m_ctx.AlivePlayers.size()));
    aliveLabel.setFillColor(sf::Color(130, 130, 170));
    m_ui.CenterText(aliveLabel, listY, 13);
    m_ctx.Window.draw(aliveLabel);

    // Draw alive player names in a row
    float nameY = listY + 22.f;
    float totalWidth = 0.f;
    std::vector<sf::Text> names;
    for (const auto& name : m_ctx.AlivePlayers)
    {
        sf::Text t(m_ctx.Font);
        t.setString(name);
        t.setCharacterSize(12);
        bool isHolder = (name == m_ctx.BombHolder);
        t.setFillColor(isHolder ? sf::Color(255, 80, 60) : sf::Color(180, 180, 200));
        if (isHolder) t.setStyle(sf::Text::Bold);
        sf::FloatRect b = t.getLocalBounds();
        totalWidth += b.size.x + 15.f;
        names.push_back(std::move(t));
    }

    float startX = m_ctx.CenterX - totalWidth / 2.f;
    for (auto& t : names)
    {
        t.setPosition({startX, nameY});
        m_ctx.Window.draw(t);
        startX += t.getLocalBounds().size.x + 15.f;
    }
}
