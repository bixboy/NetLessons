#include "Screens/ScreenLobby.h"
#include "Screens/ScreenIpConfig.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include "Avatars/AvatarManager.h"


ScreenLobby::ScreenLobby(ClientContext& ctx, UIRenderer& ui, AvatarManager& avatars)
    : m_ctx(ctx), m_ui(ui), m_avatars(avatars)
{
}

void ScreenLobby::HandleInput(const sf::Event& event)
{
    // Lobby has no special key input beyond avatar movement (handled by AvatarManager)
}

void ScreenLobby::Draw()
{
    // --- Player List Panel ---
    m_ui.DrawPanel(20.f, 20.f, 200.f, m_ctx.WindowSize.y - 40.f, sf::Color(20, 20, 30, 230));

    sf::Text listTitle(m_ctx.Font);
    listTitle.setString("JOUEURS");
    listTitle.setFillColor(sf::Color(0, 200, 255));
    listTitle.setCharacterSize(18);
    listTitle.setPosition({40.f, 40.f});
    listTitle.setStyle(sf::Text::Bold);
    m_ctx.Window.draw(listTitle);

    sf::RectangleShape sep({160.f, 1.f});
    sep.setPosition({40.f, 70.f});
    sep.setFillColor(sf::Color(60, 60, 80));
    m_ctx.Window.draw(sep);

    float y = 90.f;
    for (const auto& name : m_ctx.PlayerNames)
    {
        sf::Text pName(m_ctx.Font);
        pName.setString(name);

        bool isMe = (name == m_ctx.PseudoInput);
        pName.setFillColor(isMe ? sf::Color(100, 255, 150) : sf::Color(200, 200, 200));

        if (isMe)
            pName.setStyle(sf::Text::Bold);

        pName.setCharacterSize(15);
        pName.setPosition({40.f, y});
        m_ctx.Window.draw(pName);

        if (isMe)
        {
            sf::Text youTag(m_ctx.Font);
            youTag.setString("(vous)");
            youTag.setFillColor(sf::Color(100, 100, 130));
            youTag.setCharacterSize(11);
            youTag.setPosition({45.f + pName.getLocalBounds().size.x + 5.f, y + 2.f});
            m_ctx.Window.draw(youTag);
        }

        y += 28.f;
    }

    sf::Text countText(m_ctx.Font);
    countText.setString(std::to_string(m_ctx.PlayerNames.size()) + " joueur(s) connecte(s)");
    countText.setFillColor(sf::Color(100, 100, 130));
    countText.setCharacterSize(12);
    countText.setPosition({40.f, m_ctx.WindowSize.y - 70.f});
    m_ctx.Window.draw(countText);

    // --- Main Area ---
    sf::Text title(m_ctx.Font);
    title.setString("SALLE D'ATTENTE");
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    m_ui.CenterText(title, 60, 36);
    m_ctx.Window.draw(title);

    sf::Text status(m_ctx.Font);
    status.setString(m_ctx.ServerMessage);
    status.setFillColor(m_ctx.MessageColor);
    m_ui.CenterText(status, 130, 18);
    m_ctx.Window.draw(status);

    // --- Mini-Game Buttons ---
    float startY = 220.f;
    float btnGap = 60.f;

    if (m_ctx.IsGameRunning)
    {
        // --- SPECTATE MODE BUTTON ---
        sf::Text runningMsg(m_ctx.Font);
        runningMsg.setString("UNE PARTIE EST EN COURS !");
        runningMsg.setFillColor(sf::Color(255, 100, 100));
        runningMsg.setStyle(sf::Text::Bold);
        m_ui.CenterText(runningMsg, startY, 20);
        m_ctx.Window.draw(runningMsg);

        m_ui.DrawButton("OBSERVER  [E]", startY + 50.f, true);
    }
    else
    {
        // --- GAME SELECTION CHECKBOXES ---
        for (size_t i = 0; i < m_ctx.AvailableGames.size(); ++i)
        {
            const auto& game = m_ctx.AvailableGames[i];
            std::string label = game.Name + "  [E]";
            float btnY = startY + i * btnGap;
            m_ui.DrawButton(label, btnY, true);
        }

        sf::Text waiting(m_ctx.Font);
        waiting.setString("En attente de joueurs...");
        waiting.setFillColor(sf::Color(100, 100, 130));
        m_ui.CenterText(waiting, startY + m_ctx.AvailableGames.size() * btnGap + 30.f, 14);
        m_ctx.Window.draw(waiting);
    }

    // --- Interaction Progress Bar ---
    float interactionTimer = m_avatars.GetInteractionTimer();
    if (interactionTimer > 0.f)
    {
        sf::Vector2f pos = m_avatars.GetLocalAvatarPosition();
        float progress = interactionTimer / 3.0f;

        sf::RectangleShape bg({40.f, 6.f});
        bg.setOrigin({20.f, 3.f});
        bg.setPosition({pos.x, pos.y - 30.f});
        bg.setFillColor(sf::Color(50, 50, 50));
        m_ctx.Window.draw(bg);

        sf::RectangleShape bar({40.f * progress, 6.f});
        bar.setOrigin({20.f, 3.f});
        bar.setPosition({pos.x, pos.y - 30.f});
        bar.setFillColor(sf::Color::Yellow);
        m_ctx.Window.draw(bar);
    }
}
