#include "MiniGames/MiniGameHotPotato.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include "Avatars/AvatarManager.h"
#include <cmath>
#include <algorithm>
#include <string>

void MiniGameHotPotato::Draw(ClientContext& m_ctx, UIRenderer& m_ui)
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
    m_ctx.Target->draw(title);

    // ========== BIG TIMER ==========
    float timerY = 100.f + offset;
    float timerRadius = 90.f;

    sf::CircleShape timerBg(timerRadius);
    timerBg.setOrigin({timerRadius, timerRadius});
    timerBg.setPosition({m_ctx.CenterX, timerY + timerRadius});
    timerBg.setFillColor(sf::Color(20, 20, 30));
    timerBg.setOutlineThickness(6.f);

    float timerVal = (std::max)(0.f, m_ctx.BombTimer);
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
    m_ctx.Target->draw(timerBg);

    // Timer number
    sf::Text timerText(m_ctx.Font);
    int displaySeconds = static_cast<int>(std::ceil(timerVal));
    
    timerText.setString(std::to_string(displaySeconds));
    timerText.setCharacterSize(70);
    timerText.setFillColor(timerVal < 3.f ? sf::Color(255, 80, 60) : sf::Color::White);
    timerText.setStyle(sf::Text::Bold);
    
    sf::FloatRect tBounds = timerText.getLocalBounds();
    timerText.setPosition({m_ctx.CenterX - tBounds.size.x / 2.f, timerY + timerRadius - tBounds.size.y});
    m_ctx.Target->draw(timerText);

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
    m_ctx.Target->draw(holderLabel);

    // ========== STATUS MESSAGE ==========
    if (isBombHolder)
    {
        float bounce = std::sin(time * 4.f) * 3.f;
        sf::Text pushHint(m_ctx.Font);
        pushHint.setString("PUSH quelqu'un pour passer la bombe ! [SPACE]");
        pushHint.setFillColor(sf::Color(255, 150, 100));
        pushHint.setStyle(sf::Text::Bold);
        m_ui.CenterText(pushHint, infoY + 35.f + bounce, 14);
        m_ctx.Target->draw(pushHint);
    }
    else
    {
        sf::Text safeHint(m_ctx.Font);
        safeHint.setString("Evite le porteur de la bombe !");
        safeHint.setFillColor(sf::Color(100, 200, 100));
        m_ui.CenterText(safeHint, infoY + 35.f, 14);
        m_ctx.Target->draw(safeHint);
    }

    // ========== ALIVE PLAYERS ==========
    float listY = infoY + 70.f;
    sf::Text aliveLabel(m_ctx.Font);
    aliveLabel.setString("Survivants: " + std::to_string(m_ctx.AlivePlayers.size()));
    aliveLabel.setFillColor(sf::Color(130, 130, 170));
    m_ui.CenterText(aliveLabel, listY, 13);
    m_ctx.Target->draw(aliveLabel);

    float nameY = listY + 22.f;
    float totalWidth = 0.f;
    
    std::vector<sf::Text> names;
    for (const auto& name : m_ctx.AlivePlayers)
    {
        bool isHolder = (name == m_ctx.BombHolder);
        
        sf::Text t(m_ctx.Font);
        t.setString(name);
        t.setCharacterSize(12);
        t.setFillColor(isHolder ? sf::Color(255, 80, 60) : sf::Color(180, 180, 200));
        
        if (isHolder) 
            t.setStyle(sf::Text::Bold);
        
        sf::FloatRect b = t.getLocalBounds();
        totalWidth += b.size.x + 15.f;
        names.push_back(std::move(t));
    }

    float startX = m_ctx.CenterX - totalWidth / 2.f;
    for (auto& t : names)
    {
        t.setPosition({startX, nameY});
        m_ctx.Target->draw(t);
        startX += t.getLocalBounds().size.x + 15.f;
    }
}

void MiniGameHotPotato::OnPacket(const PacketGameData& pkt)
{
    if (pkt.Value == static_cast<int>(EGameDataType::BombElimination))
    {
        std::string eliminated = pkt.ExtraData;
        if (m_ctx.Avatars)
        {
            m_ctx.Avatars->TriggerExplosion(eliminated);
            std::cout << "[HOT POTATO] Explosion triggered for " << eliminated << std::endl;
        }
    }
}
