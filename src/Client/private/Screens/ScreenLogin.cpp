#include "Screens/ScreenLogin.h"
#include "Screens/ScreenIpConfig.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include "UI/ChatBox.h"
#include "Avatars/AvatarManager.h"


ScreenLogin::ScreenLogin(ClientContext& ctx, UIRenderer& ui, ChatBox& chat, AvatarManager& avatars)
    : m_ctx(ctx), m_ui(ui), m_chat(chat), m_avatars(avatars)
{
}

void ScreenLogin::HandleInput(const sf::Event& event)
{
    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>())
    {
        if (textEvent->unicode < 128 && textEvent->unicode > 31)
        {
            if (m_ctx.PseudoInput.size() < 15)
            {
                m_ctx.PseudoInput += static_cast<char>(textEvent->unicode);
            }
        }
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyEvent->code == sf::Keyboard::Key::Backspace && !m_ctx.PseudoInput.empty())
        {
            m_ctx.PseudoInput.pop_back();
        }
        else if (keyEvent->code == sf::Keyboard::Key::Enter && !m_ctx.PseudoInput.empty())
        {
            PacketConnectionState loginPkt;
            loginPkt.IsConnected = true;
            loginPkt.Pseudo = m_ctx.PseudoInput;
            m_ctx.Network.Send(loginPkt);

            m_ctx.State = ClientState::Lobby;
            m_ctx.ServerMessage = "Bienvenue " + m_ctx.PseudoInput + " !";
            m_ctx.PlayerNames.push_back(m_ctx.PseudoInput);
            m_ctx.Sound.Play(SoundType::Join);
            m_chat.AddMessage("Global", "", "Bienvenue dans le lobby !", MessageType::Success);

            m_avatars.InitLocalAvatar(m_ctx.PseudoInput);
        }
    }
}

void ScreenLogin::Draw()
{
    sf::Text title(m_ctx.Font);
    title.setString("IDENTIFICATION");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 80, 42);
    m_ctx.Window.draw(title);

    sf::Text subtitle(m_ctx.Font);
    subtitle.setString("Choisissez votre identité");
    subtitle.setFillColor(sf::Color(120, 120, 150));
    m_ui.CenterText(subtitle, 130, 16);
    m_ctx.Window.draw(subtitle);

    m_ui.DrawInputBox("VOTRE PSEUDO", m_ctx.PseudoInput, 250.f, true);
    m_ui.DrawButton("REJOINDRE LE LOBBY", 350.f, !m_ctx.PseudoInput.empty());
}
