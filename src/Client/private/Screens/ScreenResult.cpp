#include "Screens/ScreenResult.h"
#include "Screens/ScreenIpConfig.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"


ScreenResult::ScreenResult(ClientContext& ctx, UIRenderer& ui) : m_ctx(ctx), m_ui(ui)
{
}

void ScreenResult::HandleInput(const sf::Event& event)
{
}

void ScreenResult::Draw()
{
    float centerY = m_ctx.WindowSize.y / 2.f;
    float offset = centerY - 225.f;

    bool isWinner = (m_ctx.WinnerName == "TOI !");

    sf::Text title(m_ctx.Font);
    title.setString(isWinner ? "VICTOIRE !" : "PARTIE TERMINEE");
    title.setFillColor(isWinner ? sf::Color(100, 255, 150) : sf::Color(255, 100, 100));
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 100.f + offset, 48);
    m_ctx.Target->draw(title);

    sf::Text winnerLabel(m_ctx.Font);
    winnerLabel.setString("Gagnant");
    winnerLabel.setFillColor(sf::Color(120, 120, 150));
    m_ui.CenterText(winnerLabel, 180.f + offset, 16);
    m_ctx.Target->draw(winnerLabel);

    sf::Text winner(m_ctx.Font);
    winner.setString(m_ctx.WinnerName);
    winner.setFillColor(sf::Color::White);
    winner.setStyle(sf::Text::Bold);
    m_ui.CenterText(winner, 220.f + offset, 32);
    m_ctx.Target->draw(winner);

    m_ui.DrawButton("REJOUER  [ENTREE]", 350.f + offset, true);
}
