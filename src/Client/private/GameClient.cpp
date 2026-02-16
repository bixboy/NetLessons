#include "GameClient.h"
#include <algorithm>
#include <iostream>
#include <cmath>


GameClient::GameClient()
    : m_ui(m_ctx),
      m_avatars(m_ctx),
      m_screenIpConfig(m_ctx, m_ui),
      m_screenLogin(m_ctx, m_ui, m_chat, m_avatars),
      m_screenLobby(m_ctx, m_ui, m_avatars),
      m_screenGame(m_ctx, m_ui),
      m_screenResult(m_ctx, m_ui)
{
    m_ctx.Sound.Init();

    m_ctx.AvailableGames.push_back({0, "LE JUSTE PRIX", "Devinez le nombre mystere !"});
    m_ctx.AvailableGames.push_back({1, "BOMBE", "Push la bombe aux autres !"});

    SetupNetworkHandlers();
}

GameClient::~GameClient()
{
}

void GameClient::SetupNetworkHandlers()
{
    // GAME DATA (PLUS / MOINS + BOMB STATE)
    m_ctx.Network.OnPacket(OpCode::GameData, [this](GamePacket& rawPkt)
    {
        PacketGameData pkt;
        pkt.Deserialize(rawPkt);

        if (m_ctx.ActiveGameID == 0)
        {
            // Juste Prix hints
            m_ctx.GuessAttempts++;
            if (pkt.Value == static_cast<int>(EGameDataType::JustePrixHintUp)) {
                m_ctx.ServerMessage = "C'est PLUS (+)";
                m_ctx.MessageColor = sf::Color(100, 255, 150);
                m_ctx.LastHint = 1; m_ctx.PulseValue = 1.f;
            } else if (pkt.Value == static_cast<int>(EGameDataType::JustePrixHintDown)) {
                m_ctx.ServerMessage = "C'est MOINS (-)";
                m_ctx.MessageColor = sf::Color(255, 100, 100);
                m_ctx.LastHint = 2; m_ctx.PulseValue = 1.f;
            }
        }
        else if (m_ctx.ActiveGameID == 1)
        {
            // Hot Potato
            if (pkt.Value == static_cast<int>(EGameDataType::BombState)) {
                m_ctx.BombHolder = pkt.ExtraData;
                m_ctx.BombTimer = pkt.Timer;
            } else if (pkt.Value == static_cast<int>(EGameDataType::BombElimination)) {
                std::string eliminated = pkt.ExtraData;
                auto it = std::remove(m_ctx.AlivePlayers.begin(), m_ctx.AlivePlayers.end(), eliminated);
                if (it != m_ctx.AlivePlayers.end()) m_ctx.AlivePlayers.erase(it, m_ctx.AlivePlayers.end());

                if (eliminated == m_ctx.PseudoInput)
                    m_ctx.Sound.Play(SoundType::Lose);
                else
                    m_ctx.Sound.Play(SoundType::Leave);
            }
        }

        // Global Game State Update
        if (pkt.Value == static_cast<int>(EGameDataType::GameStateChanged))
        {
            m_ctx.IsGameRunning = (pkt.ExtraData == "1");
            
            if (m_ctx.IsGameRunning)
            {
                m_ctx.ActiveGameID = (int)pkt.Timer;
            }
            else
            {
                // Game Stopped -> Reset everything
                m_ctx.State = ClientState::Lobby;
                m_ctx.CurrentNumberChoice = 0;
                m_ctx.LocalPlayerState = EPlayerState::Lobby;
                m_ctx.ServerMessage = "En attente...";
                
                m_avatars.ResetAllSpectators();
            }
        }
    });

    // RESULT (VICTOIRE / DEFAITE)
    m_ctx.Network.OnPacket(OpCode::GameResult, [this](GamePacket& rawPkt)
    {
        PacketGameResult pkt;
        pkt.Deserialize(rawPkt);

        m_ctx.State = ClientState::Result;
        m_ctx.WinnerName = pkt.WinnerName;

        if (pkt.WinnerName == m_ctx.PseudoInput)
        {
            m_ctx.WinnerName = "TOI !";
            m_ctx.Sound.Play(SoundType::Win);
            m_chat.AddMessage("Global", "", "Félicitations, tu as gagné !", MessageType::Success);
        }
        else
        {
            m_ctx.Sound.Play(SoundType::Lose);
            m_chat.AddMessage("Global", "", pkt.WinnerName + " a trouvé le nombre !", MessageType::Info);
        }
    });

    // GAME START
    m_ctx.Network.OnPacket(OpCode::GameStart, [this](GamePacket& rawPkt)
    {
        PacketGameStart pkt;
        pkt.Deserialize(rawPkt);

        m_ctx.State = ClientState::Game;
        m_ctx.ActiveGameID = pkt.GameID;
        m_ctx.ServerMessage = (pkt.GameID == 1) ? "LA BOMBE EST LANCEE !" : "DEVINE LE NOMBRE !";
        m_ctx.MessageColor = sf::Color::White;
        m_ctx.CurrentNumberChoice = 50;
        m_ctx.GuessAttempts = 0;
        m_ctx.LastHint = 0;
        m_ctx.BombHolder = "";
        m_ctx.BombTimer = 0.f;
        m_ctx.Sound.Play(SoundType::Select);

        // Populate alive players for Hot Potato
        if (pkt.GameID == 1)
        {
            m_ctx.AlivePlayers = m_ctx.PlayerNames;
        }
        m_chat.AddMessage("System", "Info", "La partie commence ! (Jeu " + std::to_string(pkt.GameID) + ")", MessageType::Info);
    });

    // GAME END
    m_ctx.Network.OnPacket(OpCode::GameEnd, [this](GamePacket& rawPkt)
    {
        m_ctx.State = ClientState::Lobby;
        m_ctx.CurrentNumberChoice = 0;
        m_ctx.ServerMessage = "Partie terminée par le serveur";
        m_ctx.WinnerName = "";
        m_ctx.Sound.Play(SoundType::Leave);
        m_chat.AddMessage("System", "", "Le serveur a réinitialisé la partie.", MessageType::Info);
    });

    // CONNECTION STATE (JOIN / LEAVE)
    m_ctx.Network.OnPacket(OpCode::ConnectionState, [this](GamePacket& rawPkt)
    {
        PacketConnectionState pkt;
        pkt.Deserialize(rawPkt);

        if (pkt.IsConnected)
        {
            m_ctx.ServerMessage = pkt.Pseudo + " a rejoint !";
            m_ctx.MessageColor = sf::Color(100, 255, 200);
            m_ctx.Sound.Play(SoundType::Join);
            m_ctx.PlayerNames.push_back(pkt.Pseudo);
            m_ctx.PlayerColors[pkt.Pseudo] = ClientContext::GetColorFromID(pkt.ColorID);
            m_chat.AddMessage("System", pkt.Pseudo, "a rejoint la partie", MessageType::System);

            if (pkt.Pseudo != m_ctx.PseudoInput)
                m_avatars.AddRemoteAvatar(pkt.Pseudo, ClientContext::GetColorFromID(pkt.ColorID));
        }
        else
        {
            m_ctx.ServerMessage = pkt.Pseudo + " est parti.";
            m_ctx.MessageColor = sf::Color(255, 200, 100);
            m_ctx.Sound.Play(SoundType::Leave);
            m_chat.AddMessage("System", pkt.Pseudo, "a quitté la partie", MessageType::System);

            auto it = std::remove(m_ctx.PlayerNames.begin(), m_ctx.PlayerNames.end(), pkt.Pseudo);
            if (it != m_ctx.PlayerNames.end())
                m_ctx.PlayerNames.erase(it, m_ctx.PlayerNames.end());

            m_ctx.PlayerColors.erase(pkt.Pseudo);
            m_avatars.RemoveRemoteAvatar(pkt.Pseudo);
        }
    });

    // PLAYER LIST
    m_ctx.Network.OnPacket(OpCode::PlayerList, [this](GamePacket& rawPkt)
    {
        PacketPlayerList pkt;
        pkt.Deserialize(rawPkt);
        m_ctx.PlayerNames.push_back(pkt.Pseudo);
        m_ctx.PlayerColors[pkt.Pseudo] = ClientContext::GetColorFromID(pkt.ColorID);

        if (pkt.Pseudo != m_ctx.PseudoInput)
        {
            m_avatars.AddRemoteAvatar(pkt.Pseudo, ClientContext::GetColorFromID(pkt.ColorID));
            // Hide if spectating or in a different world than local player
            bool hidden = (pkt.State == EPlayerState::Spectating) || (pkt.State != m_ctx.LocalPlayerState);
            m_avatars.SetSpectator(pkt.Pseudo, hidden);
        }
    });

    m_ctx.Network.OnPacket(OpCode::PlayerState, [this](GamePacket& rawPkt) {
        PacketPlayerState pkt;
        pkt.Deserialize(rawPkt);

        if (pkt.Pseudo == m_ctx.PseudoInput)
        {
            m_ctx.LocalPlayerState = pkt.State;
            if (pkt.State == EPlayerState::Spectating)
                m_ctx.ServerMessage = "MODE SPECTATEUR ACTIF";
        }
        else
        {
            // Remote player: hide if spectating or in a different world than us
            bool hidden = (pkt.State == EPlayerState::Spectating) || (pkt.State != m_ctx.LocalPlayerState);
            m_avatars.SetSpectator(pkt.Pseudo, hidden);
        }
    });

    // CHAT
    m_ctx.Network.OnPacket(OpCode::Chat, [this](GamePacket& rawPkt)
    {
        PacketChat pkt;
        pkt.Deserialize(rawPkt);
        if (!pkt.Target.empty())
        {
            std::string prefix;
            if (pkt.Sender == m_ctx.PseudoInput)
                prefix = "A " + pkt.Target;
            else
                prefix = "De " + pkt.Sender;

            m_chat.AddMessage(pkt.ChannelName, prefix, pkt.Message, MessageType::Whisper);
        }
        else
        {
            sf::Color senderColor = sf::Color(130, 180, 255);
            if (m_ctx.PlayerColors.count(pkt.Sender))
                senderColor = m_ctx.PlayerColors[pkt.Sender];

            m_chat.AddMessage(pkt.ChannelName, pkt.Sender, pkt.Message, MessageType::Normal, senderColor);
        }
    });

    // PLAYER ACTION (Push)
    m_ctx.Network.OnPacket(OpCode::PlayerAction, [this](GamePacket& rawPkt)
    {
        PacketPlayerAction pkt;
        pkt.Deserialize(rawPkt);

        if (pkt.ActionType == 0) // Push
        {
            m_avatars.OnPushAction(pkt.Pseudo);
            m_ctx.Sound.Play(SoundType::Push);
        }
    });

    // PLAYER POSITION (Server-authoritative)
    m_ctx.Network.OnPacket(OpCode::PlayerPosition, [this](GamePacket& rawPkt)
    {
        PacketPlayerPosition pkt;
        pkt.Deserialize(rawPkt);

        if (pkt.Pseudo == m_ctx.PseudoInput)
            m_avatars.SetLocalTarget(pkt.X, pkt.Y);
        else
            m_avatars.SetAvatarTarget(pkt.Pseudo, pkt.X, pkt.Y);
    });

    // Chat send callback
    m_chat.SetOnSendMessage([this](const std::string& msg)
    {
        PacketChat pkt;

        // Command /lobby
        if (msg == "/lobby")
        {
            if (m_ctx.State == ClientState::Game || m_ctx.State == ClientState::Result)
            {
                m_ctx.State = ClientState::Lobby;
                m_ctx.CurrentNumberChoice = 0;
                m_ctx.ServerMessage = "";
                m_ctx.WinnerName = "";
                m_chat.AddMessage("System", "", "Retour au lobby.", MessageType::Info);

                 PacketPlayerState pState;
                pState.State = EPlayerState::Lobby;
                m_ctx.Network.Send(pState);
                return;
            }
            m_chat.AddMessage("System", "", "Commande impossible ici.", MessageType::Error);
            return;
        }

        // Command /Whisper
        if (msg.rfind("/w ", 0) == 0)
        {
            size_t firstSpace = msg.find(' ');
            size_t secondSpace = msg.find(' ', firstSpace + 1);

            if (secondSpace != std::string::npos)
            {
                pkt.Target = msg.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                pkt.Message = msg.substr(secondSpace + 1);
            }
            else
            {
                m_chat.AddMessage("System", "", "Usage: /w <pseudo> <message>", MessageType::Error);
                return;
            }
        }
        else if (msg[0] == '/')
        {
            m_chat.AddMessage("System", "", "Commande inconnue: " + msg, MessageType::Error);
            return;
        }
        else
        {
            pkt.Message = msg;
        }

        pkt.ChannelName = m_chat.GetActiveChannel();
        m_ctx.Network.Send(pkt);
    });
}

void GameClient::UpdateLayout()
{
    float chatWidth = (std::min)(500.f, static_cast<float>(m_ctx.WindowSize.x) - 260.f);
    float chatHeight = 160.f;
    float chatX = 240.f;
    float chatY = static_cast<float>(m_ctx.WindowSize.y) - chatHeight - 50.f;

    m_chat.SetPosition({chatX, chatY}, {chatWidth, chatHeight});
}

void GameClient::Run()
{
    m_ctx.Window.create(sf::VideoMode({900, 650}), "Le Juste Prix - Network", sf::Style::Default);
    m_ctx.Window.setFramerateLimit(60);

    m_ctx.WindowSize = m_ctx.Window.getSize();
    m_ctx.CenterX = static_cast<float>(m_ctx.WindowSize.x) / 2.f;

    if (!m_ctx.Font.openFromFile("assets/arial.ttf"))
    {
        std::cerr << "ERREUR: arial.ttf manquant !" << std::endl;
    }

    m_chat.Setup(m_ctx.Font, {240.f, 440.f}, {500.f, 160.f});
    m_chat.AddChannel("Game");
    m_chat.AddChannel("System");

    UpdateLayout();

    // --- MAIN LOOP ---
    while (m_ctx.Window.isOpen())
    {
        m_ctx.Network.PollEvents();
        m_ctx.PulseValue = (std::sin(m_ctx.AnimClock.getElapsedTime().asSeconds() * 3.f) + 1.f) / 2.f;

        // Connection watchdog
        if (m_ctx.State != ClientState::IpConfig && !m_ctx.Network.IsConnected())
        {
            m_ctx.ServerMessage = "Connexion perdue.";
            m_ctx.MessageColor = sf::Color(255, 100, 100);
            m_ctx.State = ClientState::IpConfig;
            m_chat.AddMessage("Global", "", "Connexion au serveur perdue", MessageType::Error);
        }
        else if (m_ctx.Network.IsConnected() && m_ctx.PingClock.getElapsedTime().asSeconds() > 1.0f)
        {
            PacketPing ping;
            m_ctx.Network.Send(ping);
            m_ctx.PingClock.restart();
        }

        // Event handling
        while (const auto event = m_ctx.Window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                m_ctx.Window.close();
            }
            else if (event->is<sf::Event::Resized>())
            {
                m_ctx.WindowSize = m_ctx.Window.getSize();
                m_ctx.CenterX = static_cast<float>(m_ctx.WindowSize.x) / 2.f;

                sf::View view(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_ctx.WindowSize.x), static_cast<float>(m_ctx.WindowSize.y)}));
                m_ctx.Window.setView(view);

                UpdateLayout();
            }
            else
            {
                // Chat consumes input first
                m_chat.HandleInput(*event);
                if (m_chat.IsTyping())
                    continue;

                // Dispatch to active screen
                switch (m_ctx.State)
                {
                case ClientState::IpConfig: m_screenIpConfig.HandleInput(*event); break;
                case ClientState::Login:    m_screenLogin.HandleInput(*event);    break;
                case ClientState::Lobby:    m_screenLobby.HandleInput(*event);    break;
                case ClientState::Game:     m_screenGame.HandleInput(*event);     break;
                case ClientState::Result:   m_screenResult.HandleInput(*event);   break;
                }
            }
        }

        // Avatar update (Lobby / Game / Result)
        float dt = m_dtClock.restart().asSeconds();
        if (m_ctx.State == ClientState::Lobby || m_ctx.State == ClientState::Game || m_ctx.State == ClientState::Result)
        {
            bool hasFocus = m_ctx.Window.hasFocus();
            bool canInput = hasFocus && !m_chat.IsTyping();

            if (canInput)
                m_avatars.SendInput();

            m_avatars.Update(dt, canInput);
            m_avatars.BuildInteractionZones();
            m_avatars.CheckInteraction(dt, m_chat);
        }

        // --- Rendering ---
        m_ctx.Window.clear(sf::Color(12, 12, 18));
        m_ui.DrawBackground();

        switch (m_ctx.State)
        {
        case ClientState::IpConfig: m_screenIpConfig.Draw(); break;
        case ClientState::Login:    m_screenLogin.Draw();    break;
        case ClientState::Lobby:    m_screenLobby.Draw();    break;
        case ClientState::Game:     m_screenGame.Draw();     break;
        case ClientState::Result:   m_screenResult.Draw();   break;
        }

        if (m_ctx.State == ClientState::Lobby || m_ctx.State == ClientState::Game || m_ctx.State == ClientState::Result)
        {
            m_avatars.Draw(m_ctx.Font);
        }

        if (m_ctx.State != ClientState::IpConfig && m_ctx.State != ClientState::Login)
        {
            m_chat.Draw(m_ctx.Window);
        }

        m_ctx.Window.display();
    }
}
