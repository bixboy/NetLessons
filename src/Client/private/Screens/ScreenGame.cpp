#include "Screens/ScreenGame.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"

#include "MiniGames/MiniGameJustePrix.h"
#include "MiniGames/MiniGameHotPotato.h"
#include "MiniGames/MiniGameRedLightGreenLight.h"
#include "MiniGames/MiniGameColorMatch.h"
#include <iostream>


ScreenGame::ScreenGame(ClientContext& ctx, UIRenderer& ui): m_ctx(ctx), m_ui(ui)
{
}

void ScreenGame::HandleInput(const sf::Event& event)
{
}

void ScreenGame::Update(float dt)
{
    if (m_currentGame)
    {
        m_currentGame->Update(dt);
    }
}

void ScreenGame::Draw()
{
    CheckGameSwitch();

    if (m_currentGame)
    {
        m_currentGame->Draw(m_ctx, m_ui);
    }

    if (m_ctx.LocalPlayerState == EPlayerState::Dead)
    {
         m_ui.DrawEliminationScreen();
    }

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
        
        m_ctx.Target->draw(bg);
        m_ctx.Target->draw(spec);
    }
}

void ScreenGame::CheckGameSwitch()
{
    if (m_ctx.ActiveGameID != m_lastGameID)
    {
        std::cout << "[ScreenGame] Switching to game ID: " << m_ctx.ActiveGameID << std::endl;
        m_lastGameID = m_ctx.ActiveGameID;
        m_currentGame.reset();

        if (m_lastGameID == 0)
        {
            m_currentGame = std::make_unique<MiniGameJustePrix>(m_ctx);
        }
        else if (m_lastGameID == 1)
        {
            m_currentGame = std::make_unique<MiniGameHotPotato>(m_ctx);
        }
        else if (m_lastGameID == 2)
        {
            m_currentGame = std::make_unique<MiniGameRedLightGreenLight>(m_ctx); // Keep existing
        }
        else if (m_lastGameID == 3)
        {
            m_currentGame = std::make_unique<MiniGameColorMatch>(m_ctx);
        }
    }
}

void ScreenGame::OnPacket(const PacketGameData& pkt)
{
    CheckGameSwitch();
    if (m_currentGame)
    {
        m_currentGame->OnPacket(pkt);
    }
}
