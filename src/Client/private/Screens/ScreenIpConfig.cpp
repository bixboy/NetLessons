#include "Screens/ScreenIpConfig.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"


ScreenIpConfig::ScreenIpConfig(ClientContext& ctx, UIRenderer& ui)
    : m_ctx(ctx), m_ui(ui)
{
}

void ScreenIpConfig::HandleInput(const sf::Event& event)
{
    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>())
    {
        bool isValidChar = (textEvent->unicode >= '0' && textEvent->unicode <= '9') || textEvent->unicode == '.';
        if (isValidChar && m_ctx.IpInput.size() < 15)
        {
            m_ctx.IpInput += static_cast<char>(textEvent->unicode);
        }
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyEvent->code == sf::Keyboard::Key::Backspace && !m_ctx.IpInput.empty())
        {
            m_ctx.IpInput.pop_back();
        }
        else if (keyEvent->code == sf::Keyboard::Key::Enter && !m_ctx.IpInput.empty())
        {
            m_ctx.ServerMessage = "Connexion en cours...";
            m_ctx.MessageColor = sf::Color(200, 200, 200);

            if (m_ctx.Network.Connect(m_ctx.IpInput))
            {
                m_ctx.Sound.Play(SoundType::Join);
                m_ctx.State = ClientState::Login;
                m_ctx.ServerMessage = "Connecté ! Entrez votre pseudo.";
                m_ctx.MessageColor = sf::Color(100, 255, 150);
            }
            else
            {
                m_ctx.Sound.Play(SoundType::Lose);
                m_ctx.ServerMessage = "Impossible de joindre " + m_ctx.IpInput;
                m_ctx.MessageColor = sf::Color(255, 100, 100);
            }
        }
    }
}

void ScreenIpConfig::Draw()
{
    sf::Text title(m_ctx.Font);
    title.setString("CONNEXION");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 80, 42);
    m_ctx.Window.draw(title);

    sf::Text subtitle(m_ctx.Font);
    subtitle.setString("Le Juste Prix - Multijoueur");
    subtitle.setFillColor(sf::Color(120, 120, 150));
    m_ui.CenterText(subtitle, 130, 16);
    m_ctx.Window.draw(subtitle);

    m_ui.DrawInputBox("ADRESSE IP DU SERVEUR", m_ctx.IpInput, 250.f, true);
    m_ui.DrawButton("SE CONNECTER", 350.f, !m_ctx.IpInput.empty());

    sf::Text errorMsg(m_ctx.Font);
    errorMsg.setString(m_ctx.ServerMessage);
    errorMsg.setFillColor(m_ctx.MessageColor);
    m_ui.CenterText(errorMsg, 430, 16);
    m_ctx.Window.draw(errorMsg);

    sf::Text hint(m_ctx.Font);
    hint.setString("Appuyez sur ENTREE pour valider");
    hint.setFillColor(sf::Color(80, 80, 100));
    m_ui.CenterText(hint, m_ctx.WindowSize.y - 40.f, 12);
    m_ctx.Window.draw(hint);
}
