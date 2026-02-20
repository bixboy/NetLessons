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
    m_ctx.Avatars = &m_avatars;
    m_ctx.Sound.Init();

    m_ctx.AvailableGames.push_back({0, "LE JUSTE PRIX", "Devinez le nombre mystere !"});
    m_ctx.AvailableGames.push_back({1, "BOMBE", "Push la bombe aux autres !"});
    m_ctx.AvailableGames.push_back({2, "1, 2, 3 SOLEIL", "Avance au feu vert, Stop au rouge !"});
    m_ctx.AvailableGames.push_back({3, "COLOR MATCH", "Monte sur la bonne couleur !"});

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
                
                // Trigger Visual Feedback
                m_avatars.TriggerExplosion(eliminated);

                auto it = std::remove(m_ctx.AlivePlayers.begin(), m_ctx.AlivePlayers.end(), eliminated);
                if (it != m_ctx.AlivePlayers.end()) m_ctx.AlivePlayers.erase(it, m_ctx.AlivePlayers.end());

                if (eliminated == m_ctx.PseudoInput)
                    m_ctx.Sound.Play(SoundType::Lose);
                else
                    m_ctx.Sound.Play(SoundType::Leave);
            }
        }
        else if (m_ctx.ActiveGameID == 2)
        {
            // Red Light Green Light
            if (pkt.Value == static_cast<int>(EGameDataType::GreenLight)) {
                m_ctx.IsRedLight = false;
                m_ctx.MessageColor = sf::Color::Green;
                m_ctx.ServerMessage = "FEU VERT !";
                m_ctx.Sound.Play(SoundType::Select); // Go sound
            } else if (pkt.Value == static_cast<int>(EGameDataType::RedLight)) {
                m_ctx.IsRedLight = true;
                m_ctx.MessageColor = sf::Color::Red;
                m_ctx.ServerMessage = "FEU ROUGE !";
                m_ctx.Sound.Play(SoundType::Lose);   // Stop sound (warning)
            } else if (pkt.Value == static_cast<int>(EGameDataType::PlayerEliminated)) {
                if (pkt.ExtraData == m_ctx.PseudoInput) {
                    m_chat.AddMessage("System", "", "Tu as bougé ! ELIMINE !", MessageType::Error);
                    m_ctx.Sound.Play(SoundType::Lose);
                } else {
                    m_chat.AddMessage("System", "", pkt.ExtraData + " a bougé !", MessageType::Info);
                }
            } else if (pkt.Value == static_cast<int>(EGameDataType::PlayerFinished)) {
                m_chat.AddMessage("System", "", pkt.ExtraData + " a fini !", MessageType::Success);
            } else if (pkt.Value == static_cast<int>(EGameDataType::IceModeOn)) {
                m_ctx.IsIceMode = true;
                m_avatars.SetIceMode(true);
                m_ctx.ServerMessage = "SOL GLISSANT !!!";
                m_chat.AddMessage("System", "", "Le sol est gelé !", MessageType::Info);
            } else if (pkt.Value == static_cast<int>(EGameDataType::IceModeOff)) {
                m_ctx.IsIceMode = false;
                m_avatars.SetIceMode(false);
                m_ctx.ServerMessage = "Sol normal.";
                m_chat.AddMessage("System", "", "Fini la glissade.", MessageType::Info);
            } else if (pkt.Value == static_cast<int>(EGameDataType::LightPurple)) {
                // Troll Light Purple
                m_ctx.IsRedLight = false; 
                m_ctx.ServerMessage = "FEU VIOLET !?";
                m_ctx.MessageColor = sf::Color::Magenta;
                m_ctx.TrollLight = 1; 
            } else if (pkt.Value == static_cast<int>(EGameDataType::LightOrange)) {
                // Troll Light Orange
                m_ctx.IsRedLight = false; 
                m_ctx.ServerMessage = "FEU ORANGE !?";
                m_ctx.MessageColor = sf::Color(255, 165, 0); 
                m_ctx.TrollLight = 2; 
            }

            // Clear Troll Light on Red/Green
            if (pkt.Value == static_cast<int>(EGameDataType::GreenLight) || pkt.Value == static_cast<int>(EGameDataType::RedLight)) {
                m_ctx.TrollLight = 0;
            }
        }
        else if (m_ctx.ActiveGameID == 3)
        {
            // Color Match -> Delegate to ScreenGame
            m_screenGame.OnPacket(pkt);
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
                // NEW: Clear Ice Mode and Troll Light
                m_ctx.IsIceMode = false;
                m_avatars.SetIceMode(false);
                m_ctx.TrollLight = 0;
                
                m_ctx.ServerMessage = "En attente...";
                
                m_avatars.ResetAllSpectators();
            }
        }
        else if (pkt.Value == static_cast<int>(EGameDataType::RespawnTimer))
        {
            m_ctx.RespawnTimer = pkt.Timer;
            m_ctx.ServerMessage = "EXPLOSION !";
            m_ctx.MessageColor = sf::Color::Red;
            m_chat.AddMessage("System", "", "Tu as explosé contre un mur !", MessageType::Error);
        }
    });

    // RESULT (VICTOIRE / DEFAITE)
    m_ctx.Network.OnPacket(OpCode::GameResult, [this](GamePacket& rawPkt)
    {
        PacketGameResult pkt;
        pkt.Deserialize(rawPkt);

        m_ctx.State = ClientState::Result;
        m_ctx.WinnerName = pkt.WinnerName;
        // NEW: Clear Ice Mode and Troll Light
        m_ctx.IsIceMode = false;
        m_avatars.SetIceMode(false);
        m_ctx.TrollLight = 0;

        if (pkt.WinnerName == m_ctx.PseudoInput)
        {
            m_ctx.WinnerName = "TOI !";
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
        else if (pkt.GameID == 2)
        {
            m_ctx.IsRedLight = true; // Starts Red
            m_ctx.ServerMessage = "ATTENTION...";
            m_ctx.MessageColor = sf::Color::Red;
        }
        m_chat.AddMessage("System", "Info", "La partie commence ! (Jeu " + std::to_string(pkt.GameID) + ")", MessageType::Info);
    });

    // GAME END
    m_ctx.Network.OnPacket(OpCode::GameEnd, [this](GamePacket& rawPkt)
    {
        m_ctx.State = ClientState::Lobby;
        m_ctx.CurrentNumberChoice = 0;
        m_ctx.IsIceMode = false;
        m_avatars.SetIceMode(false);
        m_ctx.TrollLight = 0;
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
            
            // Set state and update visibility
            m_avatars.SetAvatarState(pkt.Pseudo, pkt.State);
            m_avatars.UpdateVisibility(m_ctx.LocalPlayerState);
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
            
            // Local state changed -> Refresh ALL avatars visibility
            m_avatars.UpdateVisibility(m_ctx.LocalPlayerState);
        }
        else
        {
            // Remote player state changed
            m_avatars.SetAvatarState(pkt.Pseudo, pkt.State);
            m_avatars.UpdateVisibility(m_ctx.LocalPlayerState);
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
        else if (pkt.ActionType == 2) // Explosion
        {
             m_avatars.TriggerExplosion(pkt.Pseudo);
             m_ctx.Sound.Play(SoundType::Lose);
             
             if (pkt.Pseudo == m_ctx.PseudoInput)
             {
                 SpawnScreenBlood();
             }
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

    // --- Shader Init ---
    if (!m_renderTexture.resize(m_ctx.WindowSize))
    {
        std::cerr << "Failed to resize render texture!" << std::endl;
    }
    m_ctx.Target = &m_renderTexture;
    InitShaders();

    m_chat.Setup(m_ctx.Font, {240.f, 440.f}, {500.f, 160.f});
    m_chat.AddChannel("Game");
    m_chat.AddChannel("System");

    UpdateLayout();

    // --- MAIN LOOP ---
    while (m_ctx.Window.isOpen())
    {
        m_ctx.Network.PollEvents();
        m_ctx.PulseValue = (std::sin(m_ctx.AnimClock.getElapsedTime().asSeconds() * 3.f) + 1.f) / 2.f;

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

                if (!m_renderTexture.resize(m_ctx.WindowSize))
                    std::cerr << "Failed to resize render texture!" << std::endl;
                
                m_crtShader.setUniform("resolution", sf::Vector2f(static_cast<float>(m_ctx.WindowSize.x), static_cast<float>(m_ctx.WindowSize.y)));

                sf::View view(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_ctx.WindowSize.x), static_cast<float>(m_ctx.WindowSize.y)}));
                m_ctx.Window.setView(view);

                UpdateLayout();
            }
            else
            {
                m_chat.HandleInput(*event);
                if (m_chat.IsTyping())
                    continue;

                switch (m_ctx.State)
                {
                case ClientState::IpConfig:
                    m_screenIpConfig.HandleInput(*event);
                    break;
                    
                case ClientState::Login:
                    m_screenLogin.HandleInput(*event);
                    break;
                    
                case ClientState::Lobby:
                    m_screenLobby.HandleInput(*event);
                    break;
                    
                case ClientState::Game:
                    m_screenGame.HandleInput(*event);
                    break;
                    
                case ClientState::Result:
                    m_screenResult.HandleInput(*event);
                    break;
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
        
        if (m_ctx.RespawnTimer > 0.f)
            m_ctx.RespawnTimer -= dt;

        if (m_ctx.State == ClientState::Game)
            m_screenGame.Update(dt);

        // --- Rendering ---
        m_ctx.Target->clear(sf::Color(12, 12, 18));
        m_ui.DrawBackground();

        switch (m_ctx.State)
        {
        case ClientState::IpConfig:
            m_screenIpConfig.Draw();
            break;
            
        case ClientState::Login:
            m_screenLogin.Draw();
            break;
            
        case ClientState::Lobby:
            m_screenLobby.Draw();
            break;
            
        case ClientState::Game:
            m_screenGame.Draw();
            break;
            
        case ClientState::Result:
            m_screenResult.Draw();
            break;
        }

        if (m_ctx.State == ClientState::Lobby || m_ctx.State == ClientState::Game || m_ctx.State == ClientState::Result)
        {
            m_avatars.Draw(m_ctx.Font);
        }

        
        // --- DEATH SCREEN ---
        if (m_ctx.LocalPlayerState == EPlayerState::Dead && m_ctx.State == ClientState::Lobby)
        {
            sf::RectangleShape overlay(sf::Vector2f(m_ctx.WindowSize));
            overlay.setFillColor(sf::Color(20, 0, 0, 200));
            m_ctx.Target->draw(overlay);

            sf::Text deadText(m_ctx.Font);
            deadText.setString("VOUS ETES MORT");
            deadText.setCharacterSize(40);
            deadText.setFillColor(sf::Color(255, 50, 50));
            deadText.setStyle(sf::Text::Bold);
            
            sf::FloatRect bounds = deadText.getLocalBounds();
            deadText.setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
            deadText.setPosition({m_ctx.CenterX, static_cast<float>(m_ctx.WindowSize.y) / 2.f - 30.f});
            m_ctx.Target->draw(deadText);

            if (m_ctx.RespawnTimer > 0.f)
            {
                sf::Text timerText(m_ctx.Font);
                timerText.setString("Respawn dans " + std::to_string((int)std::ceil(m_ctx.RespawnTimer)) + "s");
                timerText.setCharacterSize(24);
                timerText.setFillColor(sf::Color::White);
                
                sf::FloatRect tBounds = timerText.getLocalBounds();
                timerText.setOrigin({tBounds.size.x / 2.f, tBounds.size.y / 2.f});
                timerText.setPosition({m_ctx.CenterX, static_cast<float>(m_ctx.WindowSize.y) / 2.f + 20.f});
                m_ctx.Target->draw(timerText);
            }
        }

        // --- BLOOD SPLATTERS ---
        for (auto it = m_bloodSplatters.begin(); it != m_bloodSplatters.end();)
        {
            float decay = it->decaySpeed * dt;
            it->alpha -= decay;

            if (it->alpha <= 0.f)
            {
                it = m_bloodSplatters.erase(it);
            }
            else
            {
                sf::ConvexShape splatter;
                splatter.setPointCount(6);
                splatter.setPoint(0, {0.f, 0.f});
                splatter.setPoint(1, {10.f, -5.f});
                splatter.setPoint(2, {20.f, 0.f});
                splatter.setPoint(3, {15.f, 15.f});
                splatter.setPoint(4, {0.f, 20.f});
                splatter.setPoint(5, {-10.f, 10.f});
                
                splatter.setScale(it->scale);
                splatter.setRotation(sf::degrees(it->rotation));
                splatter.setPosition(it->pos);

                sf::Color c = it->color;
                c.a = static_cast<uint8_t>(it->alpha);
                splatter.setFillColor(c);
                
                m_ctx.Target->draw(splatter);

                it->pos.y += 10.f * dt;
                
                ++it;
            }
        }

        m_renderTexture.display();
        m_ctx.Window.clear(sf::Color::Black);
        
        sf::Sprite sprite(m_renderTexture.getTexture());
        m_ctx.Window.draw(sprite, &m_crtShader);
        
        if (m_ctx.State != ClientState::IpConfig && m_ctx.State != ClientState::Login)
        {
            m_chat.Draw(m_ctx.Window);
        }
        
        m_ctx.Window.display();
    }
}

void GameClient::SpawnScreenBlood()
{
    int count = 5 + (std::rand() % 6);
    for (int i = 0; i < count; ++i)
    {
        BloodSplatter s;
        s.pos.x = static_cast<float>(std::rand() % m_ctx.WindowSize.x);
        s.pos.y = static_cast<float>(std::rand() % m_ctx.WindowSize.y);
        
        float scaleVal = 2.f + (std::rand() % 300) / 100.f;
        s.scale = {scaleVal, scaleVal};
        
        s.rotation = static_cast<float>(std::rand() % 360);
        
        s.alpha = 200.f + (std::rand() % 55);
        s.decaySpeed = 30.f + (std::rand() % 30);
        
        s.color = sf::Color(150 + (std::rand() % 50), 0, 0);
        
        m_bloodSplatters.push_back(s);
    }
}

void GameClient::InitShaders()
{
    const std::string fragmentShader = R"(
        #version 120
        uniform sampler2D texture;
        uniform vec2 resolution;
        
        void main()
        {
            vec2 uv = gl_TexCoord[0].xy;
            
            // CURVATURE
            vec2 d = uv - 0.5;
            float r = dot(d, d);
            uv = 0.5 + d * (1.0 + r * 0.04); // Reduced curvature

            if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
            {
                gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                return;
            }

            // CHROMATIC ABERRATION
            float aberration = 0.0015 * (1.0 + r * 2.0); // Reduced aberration
            float r_chan = texture2D(texture, uv - vec2(aberration, 0.0)).r;
            float g_chan = texture2D(texture, uv).g;
            float b_chan = texture2D(texture, uv + vec2(aberration, 0.0)).b;
            vec3 color = vec3(r_chan, g_chan, b_chan);

            // SCANLINES
            float scanline = sin(uv.y * resolution.y * 3.14159 * 1.5);
            color -= 0.05 * scanline; // Reduced scanline intensity

            // VIGNETTE
            float vignette = 1.0 - r * 0.8; // Reduced vignette intensity
            color *= vignette;

            // BRIGHTNESS BOOST (CRT glow)
            color *= 1.02; // Reduced brightness boost

            gl_FragColor = vec4(color, 1.0);
        }
    )";

    if (!m_crtShader.loadFromMemory(fragmentShader, sf::Shader::Type::Fragment))
    {
        std::cerr << "Failed to load CRT shader!" << std::endl;
    }
    else
    {
        m_crtShader.setUniform("texture", sf::Shader::CurrentTexture);
        m_crtShader.setUniform("resolution", sf::Vector2f(static_cast<float>(m_ctx.WindowSize.x), static_cast<float>(m_ctx.WindowSize.y)));
    }
}
