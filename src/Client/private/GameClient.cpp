#include "GameClient.h"
#include <algorithm>
#include <iostream>

#include <cmath>
#ifdef _WIN32
#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <wrl/client.h>
#endif


GameClient::GameClient() 
    : m_state(ClientState::IpConfig),
        m_ipInput("127.0.0.1"),
        m_currentNumberChoice(50), 
        m_serverMessage("Veuillez entrer l'IP"), 
        m_messageColor(sf::Color::White),
        m_pulseValue(0.f),
        m_soundSelect(m_bufferSelect),
        m_soundWin(m_bufferWin),
        m_soundLose(m_bufferLose),
        m_soundJoin(m_bufferJoin),
        m_soundLeave(m_bufferLeave)
{
    SetupNetworkHandlers();
}

GameClient::~GameClient()
{
}

void GameClient::SetupNetworkHandlers()
{    
    // GAME DATA (PLUS / MOINS)
    m_network.OnPacket(OpCode::GameData, [this](GamePacket& rawPkt) 
    {
        PacketGameData pkt;
        pkt.Deserialize(rawPkt);
        
        if (pkt.Value == 1) 
        {
            m_serverMessage = "C'est PLUS (+)"; 
            m_messageColor = sf::Color(255, 100, 100);
        } 
        else if (pkt.Value == 2) 
        {
            m_serverMessage = "C'est MOINS (-)"; 
            m_messageColor = sf::Color(200, 100, 255);
        }
    });

    // RESULT (VICTOIRE / DEFAITE)
    m_network.OnPacket(OpCode::GameResult, [this](GamePacket& rawPkt) 
    {
        PacketGameResult pkt;
        pkt.Deserialize(rawPkt);

        m_state = ClientState::Result;
        m_winnerName = pkt.WinnerName;

        if (pkt.WinnerName == m_pseudoInput)
        {
             m_winnerName = "TOI !";
             SetMuteState(false);
             SetWindowVolume(100.f);
             PlaySound(m_soundWin);
             m_chat.AddMessage("Global", "", "Félicitations, tu as gagné !", MessageType::Success);
        }
        else
        {
             PlaySound(m_soundLose);
             m_chat.AddMessage("Global", "", pkt.WinnerName + " a trouvé le nombre !", MessageType::Info);
        }
    });

    // GAME START
    m_network.OnPacket(OpCode::GameStart, [this](GamePacket& rawPkt) 
    {
        m_state = ClientState::Game;
        m_serverMessage = "DEVINE LE NOMBRE !";
        m_messageColor = sf::Color::White;
        m_currentNumberChoice = 50;
        PlaySound(m_soundSelect);
        m_chat.AddMessage("Global", "", "La partie commence !", MessageType::Info);
    });

    // GAME END
    m_network.OnPacket(OpCode::GameEnd, [this](GamePacket& rawPkt)
    {
         m_state = ClientState::Lobby;
         m_currentNumberChoice = 0;
         m_serverMessage = "Partie terminée par le serveur";
         m_winnerName = "";
         PlaySound(m_soundLeave);
         m_chat.AddMessage("System", "", "Le serveur a réinitialisé la partie.", MessageType::Info);
    });

    // CONNECTION STATE (JOIN / LEAVE)
    m_network.OnPacket(OpCode::ConnectionState, [this](GamePacket& rawPkt) 
    {
        PacketConnectionState pkt;
        pkt.Deserialize(rawPkt);

        if (pkt.IsConnected)
        {
             m_serverMessage = pkt.Pseudo + " a rejoint !";
             m_messageColor = sf::Color(100, 255, 200);
             PlaySound(m_soundJoin);
             m_playerNames.push_back(pkt.Pseudo);
             m_playerColors[pkt.Pseudo] = GetColorFromID(pkt.ColorID);
             m_chat.AddMessage("System", pkt.Pseudo, "a rejoint la partie", MessageType::System);
        }
        else
        {
             m_serverMessage = pkt.Pseudo + " est parti.";
             m_messageColor = sf::Color(255, 200, 100);
             PlaySound(m_soundLeave);
             m_chat.AddMessage("System", pkt.Pseudo, "a quitté la partie", MessageType::System);
             
             auto it = std::remove(m_playerNames.begin(), m_playerNames.end(), pkt.Pseudo);
            
             if (it != m_playerNames.end())
                 m_playerNames.erase(it, m_playerNames.end());
             
             m_playerColors.erase(pkt.Pseudo);
        }
    });

    // PLAYER LIST
    m_network.OnPacket(OpCode::PlayerList, [this](GamePacket& rawPkt) 
    {
        PacketPlayerList pkt;
        pkt.Deserialize(rawPkt);
        m_playerNames.push_back(pkt.Pseudo);
        m_playerColors[pkt.Pseudo] = GetColorFromID(pkt.ColorID);
    });

    // CHAT
    m_network.OnPacket(OpCode::Chat, [this](GamePacket& rawPkt) 
    {
        PacketChat pkt;
        pkt.Deserialize(rawPkt);
        if (!pkt.Target.empty())
        {
             // Whisper
             std::string prefix;
             if (pkt.Sender == m_pseudoInput)
                prefix = "A " + pkt.Target;
             else
                prefix = "De " + pkt.Sender;

             m_chat.AddMessage(pkt.ChannelName, prefix, pkt.Message, MessageType::Whisper);
        }
        else
        {
             sf::Color senderColor = sf::Color(130, 180, 255);
             if (m_playerColors.count(pkt.Sender))
                 senderColor = m_playerColors[pkt.Sender];

             m_chat.AddMessage(pkt.ChannelName, pkt.Sender, pkt.Message, MessageType::Normal, senderColor);
        }
    });

    // Envoi du chat
    m_chat.SetOnSendMessage([this](const std::string& msg)
    {
        PacketChat pkt;
        
        // Command /lobby
        if (msg == "/lobby")
        {
             if (m_state == ClientState::Game || m_state == ClientState::Result)
             {
                 m_state = ClientState::Lobby;
                 m_currentNumberChoice = 0;
                 m_serverMessage = "";
                 m_winnerName = "";
                 m_chat.AddMessage("System", "", "Retour au lobby.", MessageType::Info);
                 
                 PacketPlayerState pState;
                 pState.IsSpectator = true;
                 m_network.Send(pState);

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
        }
        else
        {
             pkt.Message = msg;
        }

        pkt.ChannelName = m_chat.GetActiveChannel();
        m_network.Send(pkt);
    });
}

void GameClient::Run()
{
    m_window.create(sf::VideoMode({900, 650}), "Le Juste Prix - Network", sf::Style::Default);
    m_window.setFramerateLimit(60);

    m_windowSize = m_window.getSize();
    m_centerX = static_cast<float>(m_windowSize.x) / 2.f;

    if (!m_font.openFromFile("assets/arial.ttf"))
    {
        std::cerr << "ERREUR: arial.ttf manquant !" << std::endl;
    }

    m_chat.Setup(m_font, {240.f, 440.f}, {500.f, 160.f});
    m_chat.AddChannel("Game");
    m_chat.AddChannel("System");
    
    UpdateLayout();
    InitSounds();

    // --- BOUCLE PRINCIPALE ---
    while (m_window.isOpen())
    {
        m_network.PollEvents();
        m_pulseValue = (std::sin(m_animClock.getElapsedTime().asSeconds() * 3.f) + 1.f) / 2.f;
        
        if (m_state != ClientState::IpConfig && !m_network.IsConnected())
        {
             m_serverMessage = "Connexion perdue.";
             m_messageColor = sf::Color(255, 100, 100);
             m_state = ClientState::IpConfig;
             m_chat.AddMessage("Global", "", "Connexion au serveur perdue", MessageType::Error);
        }
        else if (m_network.IsConnected() && m_pingClock.getElapsedTime().asSeconds() > 1.0f)
        {
            PacketPing ping;
            m_network.Send(ping);
            m_pingClock.restart();
        }

        while (const auto event = m_window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                m_window.close();
            }
            else if (event->is<sf::Event::Resized>())
            {
                m_windowSize = m_window.getSize();
                m_centerX = static_cast<float>(m_windowSize.x) / 2.f;
                
                sf::View view(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_windowSize.x), static_cast<float>(m_windowSize.y)}));
                m_window.setView(view);
                
                UpdateLayout();
            }
            else
            {
                HandleInput(*event);
            }
        }

        m_window.clear(sf::Color(12, 12, 18));
        DrawBackground();

        switch (m_state)
        {
        case ClientState::IpConfig:
            DrawIpConfig();
            break;
            
        case ClientState::Login:
            DrawLogin();
            break;
            
        case ClientState::Lobby:
            DrawLobby();
            break;
            
        case ClientState::Game:
            DrawGame();
            break;
            
        case ClientState::Result:
            DrawResult();
            break;
            
        default:
            break;
        }

        if (m_state != ClientState::IpConfig && m_state != ClientState::Login)
        {
             m_chat.Draw(m_window);
        }

        m_window.display();
    }
}

void GameClient::UpdateLayout()
{
    float chatWidth = (std::min)(500.f, static_cast<float>(m_windowSize.x) - 260.f);
    float chatHeight = 160.f;
    float chatX = 240.f;
    float chatY = static_cast<float>(m_windowSize.y) - chatHeight - 50.f;
    
    m_chat.SetPosition({chatX, chatY}, {chatWidth, chatHeight});
}


// --- AUDIO ---

void GameClient::InitSounds()
{
    if (m_bufferSelect.loadFromFile("assets/Sounds/ClickButton.mp3"))
    {
        m_soundSelect.setBuffer(m_bufferSelect);
        m_soundSelect.setVolume(70.f);
    }

    if (m_bufferWin.loadFromFile("assets/Sounds/Win.mp3"))
    {
        m_soundWin.setBuffer(m_bufferWin);
        m_soundWin.setVolume(70.f);
    }
    if (m_bufferLose.loadFromFile("assets/Sounds/Lose.mp3"))
    {
        m_soundLose.setBuffer(m_bufferLose);
        m_soundLose.setVolume(100.f);
    }

    if (m_bufferJoin.loadFromFile("assets/Sounds/Join.mp3"))
    {
        m_soundJoin.setBuffer(m_bufferJoin);
        m_soundJoin.setVolume(70.f);
    }
    if (m_bufferLeave.loadFromFile("assets/Sounds/Leave.mp3"))
    {
        m_soundLeave.setBuffer(m_bufferLeave);
        m_soundLeave.setVolume(70.f);
    }
}

void GameClient::PlaySound(sf::Sound& sound)
{
    sound.setPitch(1.f + (rand() % 20 - 10) / 100.f);
    sound.play();
}

void GameClient::SetWindowVolume(float volumeLevel)
{
#ifdef _WIN32
    volumeLevel = (std::max)(volumeLevel, 100.f);

    HRESULT hr = CoInitialize(NULL);
    bool coInitSuccess = SUCCEEDED(hr);

    if (coInitSuccess || hr == RPC_E_CHANGED_MODE)
    {
        {
            Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnumerator;
            Microsoft::WRL::ComPtr<IMMDevice> defaultDevice;
            Microsoft::WRL::ComPtr<IAudioEndpointVolume> endpointVolume;

            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
            if (SUCCEEDED(hr))
            {
                hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
                if (SUCCEEDED(hr))
                {
                    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&endpointVolume);
                    if (SUCCEEDED(hr))
                    {
                        float newVolume = volumeLevel / 100.0f;
                        endpointVolume->SetMasterVolumeLevelScalar(newVolume, NULL);
                    }
                }
            }
        }
        
        if (coInitSuccess)
        {
            CoUninitialize();
        }
    }
#endif

}

void GameClient::SetMuteState(bool state)
{
#ifdef _WIN32
    HRESULT hr = CoInitialize(NULL);
    bool coInitSuccess = SUCCEEDED(hr);

    if (coInitSuccess || hr == RPC_E_CHANGED_MODE)
    {
        {
            Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnumerator;
            Microsoft::WRL::ComPtr<IMMDevice> defaultDevice;
            Microsoft::WRL::ComPtr<IAudioEndpointVolume> endpointVolume;

            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
            if (SUCCEEDED(hr))
            {
                hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
                if (SUCCEEDED(hr))
                {
                    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&endpointVolume);
                    if (SUCCEEDED(hr))
                    {
                        endpointVolume->SetMute(state, NULL);
                    }
                }
            }
        }

        if (coInitSuccess)
        {
            CoUninitialize();
        }
    }
#endif

}


// --- GESTION DES INPUTS ---

void GameClient::HandleInput(const sf::Event& event)
{
    m_chat.HandleInput(event);
    if (m_chat.IsTyping())
        return;

    if (m_state == ClientState::IpConfig)
    {
        HandleIpInput(event);
    }
    else if (m_state == ClientState::Login)
    {
        HandleLoginInput(event);
    }
    else if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (m_state == ClientState::Lobby && keyEvent->code == sf::Keyboard::Key::S)
        {
            PacketGameStart pkt;
            m_network.Send(pkt);
            m_chat.AddMessage("Global", "", "Lancement de la partie...", MessageType::Info);
        }
        else if (m_state == ClientState::Game)
        {
            HandleGameInput(keyEvent->code);
        }
        else if (m_state == ClientState::Result && keyEvent->code == sf::Keyboard::Key::Enter)
        {
            PacketGameStart pkt;
            m_network.Send(pkt);
            m_serverMessage = "En attente du serveur...";
        }
    }
}

void GameClient::HandleIpInput(const sf::Event& event)
{
    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>())
    {
        bool isValidChar = (textEvent->unicode >= '0' && textEvent->unicode <= '9') || textEvent->unicode == '.';
        if (isValidChar && m_ipInput.size() < 15)
        {
            m_ipInput += static_cast<char>(textEvent->unicode);
        }
    }
    
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyEvent->code == sf::Keyboard::Key::Backspace && !m_ipInput.empty())
        {
            m_ipInput.pop_back();
        }
        else if (keyEvent->code == sf::Keyboard::Key::Enter && !m_ipInput.empty())
        {
            m_serverMessage = "Connexion en cours...";
            m_messageColor = sf::Color(200, 200, 200);
            
            if (m_network.Connect(m_ipInput))
            {
                PlaySound(m_soundJoin);
                m_state = ClientState::Login;
                m_serverMessage = "Connecté ! Entrez votre pseudo.";
                m_messageColor = sf::Color(100, 255, 150);
            }
            else
            {
                PlaySound(m_soundLose);
                m_serverMessage = "Impossible de joindre " + m_ipInput;
                m_messageColor = sf::Color(255, 100, 100);
            }
        }
    }
}

void GameClient::HandleLoginInput(const sf::Event& event)
{
    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>())
    {
        if (textEvent->unicode < 128 && textEvent->unicode > 31)
        {
            if (m_pseudoInput.size() < 15)
            {
                m_pseudoInput += static_cast<char>(textEvent->unicode);
            }
        }
    }
    
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyEvent->code == sf::Keyboard::Key::Backspace && !m_pseudoInput.empty())
        {
            m_pseudoInput.pop_back();
        }
        else if (keyEvent->code == sf::Keyboard::Key::Enter && !m_pseudoInput.empty())
        {
            PacketConnectionState loginPkt;
            loginPkt.IsConnected = true;
            loginPkt.Pseudo = m_pseudoInput;
            m_network.Send(loginPkt);

            m_state = ClientState::Lobby;
            m_serverMessage = "Bienvenue " + m_pseudoInput + " !";
            m_playerNames.push_back(m_pseudoInput);
            PlaySound(m_soundJoin);
            m_chat.AddMessage("Global", "", "Bienvenue dans le lobby !", MessageType::Success);
        }
    }
}

void GameClient::HandleGameInput(sf::Keyboard::Key key)
{
    if (key == sf::Keyboard::Key::Up)
    {
        if (m_currentNumberChoice < 100)
        {
            m_currentNumberChoice++;
            PlaySound(m_soundSelect);   
        }
    }
    else if (key == sf::Keyboard::Key::Down)
    {
        if (m_currentNumberChoice > 0)
        {
            m_currentNumberChoice--;
            PlaySound(m_soundSelect);
        }
    }
    else if (key == sf::Keyboard::Key::Enter)
    {
        PacketGameData pkt;
        pkt.Value = m_currentNumberChoice;
        m_network.Send(pkt);
        
        m_serverMessage = "Envoyé : " + std::to_string(m_currentNumberChoice);
        m_messageColor = sf::Color(255, 200, 100);
        PlaySound(m_soundSelect);
    }
}


// --- GESTION DE L'UI ---

void GameClient::DrawBackground()
{
    sf::RectangleShape gradient({static_cast<float>(m_windowSize.x), static_cast<float>(m_windowSize.y)});
    gradient.setPosition({0, 0});
    
    sf::Color lineColor(30, 30, 45, 100);
    for (int i = 0; i < 20; i++)
    {
        sf::RectangleShape line({static_cast<float>(m_windowSize.x), 1.f});
        line.setPosition({0, static_cast<float>(i * 40)});
        line.setFillColor(lineColor);
        m_window.draw(line);
    }
}

void GameClient::DrawPanel(float x, float y, float w, float h, sf::Color color, float cornerRadius)
{
    sf::RectangleShape panel({w, h});
    panel.setPosition({x, y});
    panel.setFillColor(color);
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(60, 60, 80, 200));
    m_window.draw(panel);
}

void GameClient::DrawButton(const std::string& textStr, float y, bool active, bool hovered)
{
    float btnWidth = 320.f;
    float btnHeight = 45.f;
    
    sf::RectangleShape btn({btnWidth, btnHeight});
    btn.setOrigin({btnWidth / 2.f, btnHeight / 2.f});
    btn.setPosition({m_centerX, y});
    
    sf::Color baseColor = active ? sf::Color(0, 180, 230) : sf::Color(50, 50, 60);
    sf::Color borderColor = active ? sf::Color(0, 220, 255) : sf::Color(80, 80, 100);
    
    if (active)
    {
        uint8_t pulse = static_cast<uint8_t>(m_pulseValue * 30);
        baseColor = sf::Color(0, 180 + pulse, 230 + pulse / 2);
    }
    
    btn.setFillColor(baseColor);
    btn.setOutlineThickness(2.f);
    btn.setOutlineColor(borderColor);
    
    m_window.draw(btn);

    sf::Text text(m_font);
    text.setString(textStr);
    text.setCharacterSize(16);
    text.setFillColor(active ? sf::Color::White : sf::Color(180, 180, 180));
    text.setStyle(sf::Text::Bold);
    CenterText(text, y - 10.f, 16);
    m_window.draw(text);
}

void GameClient::DrawInputBox(const std::string& label, const std::string& value, float y, bool active)
{
    sf::Text lbl(m_font);
    lbl.setString(label);
    lbl.setFillColor(sf::Color(150, 150, 180));
    lbl.setCharacterSize(14);
    CenterText(lbl, y - 45.f, 14);
    m_window.draw(lbl);

    float boxWidth = 320.f;
    float boxHeight = 45.f;
    
    sf::RectangleShape box({boxWidth, boxHeight});
    box.setOrigin({boxWidth / 2.f, boxHeight / 2.f});
    box.setPosition({m_centerX, y});
    box.setFillColor(sf::Color(25, 25, 35));
    box.setOutlineThickness(active ? 2.f : 1.f);
    box.setOutlineColor(active ? sf::Color(0, 200, 255) : sf::Color(80, 80, 100));
    m_window.draw(box);

    std::string displayValue = value;
    if (active)
    {
        float blinkTime = m_animClock.getElapsedTime().asSeconds();
        if (fmod(blinkTime, 1.0f) < 0.5f)
        {
            displayValue += "|";
        }
    }

    sf::Text val(m_font);
    val.setString(displayValue);
    val.setFillColor(sf::Color::White);
    val.setCharacterSize(18);
    CenterText(val, y - 10.f, 18);
    m_window.draw(val);
}

// --- ECRANS ---

void GameClient::DrawIpConfig()
{
    sf::Text title(m_font);
    title.setString("CONNEXION");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    CenterText(title, 80, 42);
    m_window.draw(title);

    sf::Text subtitle(m_font);
    subtitle.setString("Le Juste Prix - Multijoueur");
    subtitle.setFillColor(sf::Color(120, 120, 150));
    CenterText(subtitle, 130, 16);
    m_window.draw(subtitle);

    DrawInputBox("ADRESSE IP DU SERVEUR", m_ipInput, 250.f, true);
    
    DrawButton("SE CONNECTER", 350.f, !m_ipInput.empty());

    // Message de statut
    sf::Text errorMsg(m_font);
    errorMsg.setString(m_serverMessage);
    errorMsg.setFillColor(m_messageColor);
    CenterText(errorMsg, 430, 16);
    m_window.draw(errorMsg);

    // Hint
    sf::Text hint(m_font);
    hint.setString("Appuyez sur ENTREE pour valider");
    hint.setFillColor(sf::Color(80, 80, 100));
    CenterText(hint, m_windowSize.y - 40.f, 12);
    m_window.draw(hint);
}

void GameClient::DrawLogin()
{
    sf::Text title(m_font);
    title.setString("IDENTIFICATION");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    CenterText(title, 80, 42);
    m_window.draw(title);

    sf::Text subtitle(m_font);
    subtitle.setString("Choisissez votre identité");
    subtitle.setFillColor(sf::Color(120, 120, 150));
    CenterText(subtitle, 130, 16);
    m_window.draw(subtitle);

    DrawInputBox("VOTRE PSEUDO", m_pseudoInput, 250.f, true);

    DrawButton("REJOINDRE LE LOBBY", 350.f, !m_pseudoInput.empty());
}

void GameClient::DrawLobby()
{
    // Panel joueurs
    DrawPanel(20.f, 20.f, 200.f, m_windowSize.y - 40.f, sf::Color(20, 20, 30, 230));
    
    sf::Text listTitle(m_font);
    listTitle.setString("JOUEURS");
    listTitle.setFillColor(sf::Color(0, 200, 255));
    listTitle.setCharacterSize(18);
    listTitle.setPosition({40.f, 40.f});
    listTitle.setStyle(sf::Text::Bold);
    m_window.draw(listTitle);

    // Ligne de séparation
    sf::RectangleShape sep({160.f, 1.f});
    sep.setPosition({40.f, 70.f});
    sep.setFillColor(sf::Color(60, 60, 80));
    m_window.draw(sep);

    float y = 90.f;
    for (const auto& name : m_playerNames)
    {
        sf::Text pName(m_font);
        pName.setString(name);
        
        bool isMe = (name == m_pseudoInput);
        pName.setFillColor(isMe ? sf::Color(100, 255, 150) : sf::Color(200, 200, 200));
        
        if (isMe)
            pName.setStyle(sf::Text::Bold);
        
        pName.setCharacterSize(15);
        pName.setPosition({40.f, y});
        m_window.draw(pName);
        
        // Indicateur "vous"
        if (isMe)
        {
            sf::Text youTag(m_font);
            youTag.setString("(vous)");
            youTag.setFillColor(sf::Color(100, 100, 130));
            youTag.setCharacterSize(11);
            youTag.setPosition({45.f + pName.getLocalBounds().size.x + 5.f, y + 2.f});
            m_window.draw(youTag);
        }
        
        y += 28.f;
    }

    // Nombre de joueurs
    sf::Text countText(m_font);
    countText.setString(std::to_string(m_playerNames.size()) + " joueur(s) connecte(s)");
    countText.setFillColor(sf::Color(100, 100, 130));
    countText.setCharacterSize(12);
    countText.setPosition({40.f, m_windowSize.y - 70.f});
    m_window.draw(countText);

    // Zone principale
    sf::Text title(m_font);
    title.setString("SALLE D'ATTENTE");
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    CenterText(title, 60, 36);
    m_window.draw(title);

    sf::Text status(m_font);
    status.setString(m_serverMessage);
    status.setFillColor(m_messageColor);
    CenterText(status, 130, 18);
    m_window.draw(status);

    DrawButton("LANCER LA PARTIE  [S]", 220.f, true);
    
    sf::Text waiting(m_font);
    waiting.setString("En attente de joueurs...");
    waiting.setFillColor(sf::Color(100, 100, 130));
    CenterText(waiting, 290, 14);
    m_window.draw(waiting);
}

void GameClient::DrawGame()
{
    float centerY = m_windowSize.y / 2.f;
    float offset = centerY - 230.f;

    // Panel info
    DrawPanel(m_centerX - 180.f, 20.f + offset, 360.f, 80.f, sf::Color(20, 20, 30, 230));
    
    // Title
    sf::Text title(m_font);
    title.setString("TROUVE LE NOMBRE");
    title.setFillColor(sf::Color(0, 200, 255));
    title.setStyle(sf::Text::Bold);
    CenterText(title, 40.f + offset, 22);
    m_window.draw(title);

    // Resultat
    sf::Text result(m_font);
    result.setString(m_serverMessage);
    result.setFillColor(m_messageColor);
    CenterText(result, 72.f + offset, 16);
    m_window.draw(result);

    // Cercle principal
    float circleRadius = 90.f;
    sf::CircleShape circle(circleRadius);
    circle.setFillColor(sf::Color(25, 25, 35));
    circle.setOutlineThickness(4.f);
    
    // Couleur cercle
    uint8_t pulse = static_cast<uint8_t>(m_pulseValue * 50);
    circle.setOutlineColor(sf::Color(0, 180 + pulse, 230 + pulse / 2));
    
    circle.setOrigin({circleRadius, circleRadius});
    circle.setPosition({m_centerX, 230.f + offset});
    m_window.draw(circle);

    // Nombre
    sf::Text nb(m_font);
    nb.setString(std::to_string(m_currentNumberChoice));
    nb.setFillColor(sf::Color::White);
    nb.setStyle(sf::Text::Bold);
    nb.setCharacterSize(55);
    sf::FloatRect nbBounds = nb.getLocalBounds();
    nb.setPosition({m_centerX - nbBounds.size.x / 2.f, 195.f + offset});
    m_window.draw(nb);

    // Fleche haut
    sf::Text arrowUp(m_font);
    arrowUp.setString("^");
    arrowUp.setFillColor(m_currentNumberChoice < 100 ? sf::Color(100, 255, 150) : sf::Color(60, 60, 80));
    arrowUp.setCharacterSize(30);
    CenterText(arrowUp, 120.f + offset, 30);
    m_window.draw(arrowUp);

    // Fleche bas
    sf::Text arrowDown(m_font);
    arrowDown.setString("v");
    arrowDown.setFillColor(m_currentNumberChoice > 0 ? sf::Color(255, 100, 100) : sf::Color(60, 60, 80));
    arrowDown.setCharacterSize(30);
    CenterText(arrowDown, 310.f + offset, 30);
    m_window.draw(arrowDown);

    // Validation
    DrawButton("VALIDER  [ENTREE]", 400.f + offset, true);
    
    // Description
    sf::Text hint(m_font);
    hint.setString("Utilisez HAUT / BAS pour changer le nombre");
    hint.setFillColor(sf::Color(100, 100, 130));
    CenterText(hint, 460.f + offset, 13);
    m_window.draw(hint);
}

void GameClient::DrawResult()
{
    float centerY = m_windowSize.y / 2.f;
    float offset = centerY - 225.f;

    bool isWinner = (m_winnerName == "TOI !");
    
    sf::Text title(m_font);
    title.setString(isWinner ? "VICTOIRE !" : "PARTIE TERMINEE");
    title.setFillColor(isWinner ? sf::Color(100, 255, 150) : sf::Color(255, 100, 100));
    title.setStyle(sf::Text::Bold);
    CenterText(title, 100.f + offset, 48);
    m_window.draw(title);

    sf::Text winnerLabel(m_font);
    winnerLabel.setString("Gagnant");
    winnerLabel.setFillColor(sf::Color(120, 120, 150));
    CenterText(winnerLabel, 180.f + offset, 16);
    m_window.draw(winnerLabel);

    sf::Text winner(m_font);
    winner.setString(m_winnerName);
    winner.setFillColor(sf::Color::White);
    winner.setStyle(sf::Text::Bold);
    CenterText(winner, 220.f + offset, 32);
    m_window.draw(winner);

    DrawButton("REJOUER  [ENTREE]", 350.f + offset, true);
}

void GameClient::CenterText(sf::Text& text, float y, int fontSize)
{
    text.setCharacterSize(fontSize);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition({m_centerX - (bounds.size.x / 2.f), y});
}

sf::Color GameClient::GetColorFromID(uint8_t id)
{
    static const sf::Color colors[] = {
        sf::Color(255, 100, 100), // Rouge
        sf::Color(100, 255, 100), // Vert
        sf::Color(100, 100, 255), // Bleu
        sf::Color(255, 255, 100), // Jaune
        sf::Color(255, 100, 255), // Rose
        sf::Color(100, 255, 255), // Cyan
        sf::Color(255, 165, 0),   // Orange
        sf::Color(150, 100, 255)  // Violet
    };

    if (id < 8) 
        return colors[id];

    return sf::Color::White;
}
