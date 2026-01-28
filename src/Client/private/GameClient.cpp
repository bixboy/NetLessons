#include "../public/GameClient.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <mmdeviceapi.h>
#include <endpointvolume.h>


GameClient::GameClient() 
    : m_state(ClientState::IpConfig),
        m_ipInput("127.0.0.1"),
        m_currentNumberChoice(50), 
        m_serverMessage("Veuillez entrer l'IP"), 
        m_messageColor(sf::Color::White),
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
    // PLUS / MOINS
    m_network.OnPacket(PacketType::GameData, [this](GamePacket& pkt) {
        int code = 0;
        pkt >> code;
        if (code == 1) 
        {
            m_serverMessage = "C'est PLUS (+)"; m_messageColor = sf::Color::Red;
        } 
        else if (code == 2) 
        {
            m_serverMessage = "C'est MOINS (-)"; m_messageColor = sf::Color::Magenta;
        }
    });

    // VICTOIRE
    m_network.OnPacket(PacketType::Win, [this](GamePacket& pkt) {
        m_state = ClientState::Result;
        m_winnerName = "TOI (Bravo !)";
        SetMuteState(false);
        SetWindowVolume(100.f);
        PlaySound(m_soundWin);
    });

    // DEFAITE
    m_network.OnPacket(PacketType::Lose, [this](GamePacket& pkt) {
        std::string winner;
        pkt >> winner;
        m_state = ClientState::Result;
        m_winnerName = winner;
        PlaySound(m_soundLose);
    });

    // GAME START
    m_network.OnPacket(PacketType::GameStart, [this](GamePacket& pkt) {
        m_state = ClientState::Game;
        m_serverMessage = "DEVINE LE NOMBRE !";
        m_messageColor = sf::Color::White;
        PlaySound(m_soundSelect);
    });

    // JOIN
    m_network.OnPacket(PacketType::Connect, [this](GamePacket& pkt) {
        std::string pseudo;
        pkt >> pseudo;
        m_serverMessage = pseudo + " a rejoint !";
        m_messageColor = sf::Color::Cyan;
        PlaySound(m_soundJoin);
        m_playerNames.push_back(pseudo);
    });

    // LISTE
    m_network.OnPacket(PacketType::PlayerList, [this](GamePacket& pkt) {
        std::string pseudo;
        pkt >> pseudo;
        m_playerNames.push_back(pseudo);
    });

    // LEAVE
    m_network.OnPacket(PacketType::Disconnect, [this](GamePacket& pkt) {
        std::string pseudo;
        pkt >> pseudo;
        m_serverMessage = pseudo + " est parti.";
        m_messageColor = sf::Color::Yellow;
        PlaySound(m_soundLeave);
        m_chat.AddMessage("SYSTEM", pseudo + " est parti.", sf::Color::Yellow);
        
        auto it = std::remove(m_playerNames.begin(), m_playerNames.end(), pseudo);
        if (it != m_playerNames.end()) m_playerNames.erase(it, m_playerNames.end());
    });

    // CHAT
    m_network.OnPacket(PacketType::Chat, [this](GamePacket& pkt) {
        std::string sender;
        std::string msg;
        pkt >> sender >> msg;
        m_chat.AddMessage(sender, msg);
    });

    // Envoi du chat
    m_chat.SetOnSendMessage([this](const std::string& msg){
        GamePacket pkt;
        pkt << (int)PacketType::Chat << msg;
        m_network.Send(pkt);
    });
}

void GameClient::Run()
{
    m_window.create(sf::VideoMode({800, 600}), "Le Juste Prix - Network");
    m_window.setFramerateLimit(60);

    if (!m_font.openFromFile("assets/arial.ttf"))
    {
        std::cerr << "ERREUR: arial.ttf manquant !" << std::endl;
    }

    m_chat.Setup(m_font, {240.f, 400.f}, {540.f, 150.f}); // Chat moved up

    InitSounds();

    // --- BOUCLE PRINCIPALE ---
    while (m_window.isOpen())
    {
        m_network.PollEvents();
        
        if (m_state != ClientState::IpConfig && !m_network.IsConnected())
        {
             m_serverMessage = "Connexion perdue.";
             m_messageColor = sf::Color::Red;
             m_state = ClientState::IpConfig;
        }
        else if (m_network.IsConnected() && m_pingClock.getElapsedTime().asSeconds() > 1.0f)
        {
            GamePacket ping;
            ping << (int)PacketType::Ping;
            m_network.Send(ping);
            m_pingClock.restart();
        }

        while (const auto event = m_window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                m_window.close();
            }
            else
            {
                HandleInput(*event);
            }
        }

        m_window.clear(sf::Color(20, 20, 30));
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
             // Chat Background
             DrawPanel(240.f, 400.f, 540.f, 150.f, sf::Color(0, 0, 0, 150));
             m_chat.Draw(m_window);
        }

        m_window.display();
    }
}


// --- AUDIO ---

void GameClient::InitSounds()
{
    if (m_bufferSelect.loadFromFile("assets/Sounds/ClickButton.mp3"))
    {
        m_soundSelect.setBuffer(m_bufferSelect);
        m_soundSelect.setVolume(70.f);
    }

    // Win / Lose Sounds
    if (m_bufferWin.loadFromFile("assets/Sounds/Win.mp3"))
    {
        m_soundWin.setBuffer(m_bufferWin);
        m_soundWin.setVolume(70.f);
    }
    if (m_bufferLose.loadFromFile("assets/Sounds/Lose.mp3"))
    {
        m_soundLose.setBuffer(m_bufferLose);
        m_soundLose.setVolume(200.f);
    }

    // Join / Leave Sounds
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
    volumeLevel = std::max<float>(volumeLevel, 100);

    HRESULT hr;

    CoInitialize(NULL);

    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDevice* defaultDevice = NULL;
    IAudioEndpointVolume* endpointVolume = NULL;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);
    if (SUCCEEDED(hr))
    {
        hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
        if (SUCCEEDED(hr))
        {
            hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&endpointVolume);
            if (SUCCEEDED(hr))
            {
                float newVolume = volumeLevel / 100.0f;
                
                endpointVolume->SetMasterVolumeLevelScalar(newVolume, NULL);
                endpointVolume->Release();
            }
            defaultDevice->Release();
        }
        deviceEnumerator->Release();
    }

    CoUninitialize();
}

void GameClient::SetMuteState(bool state)
{
    CoInitialize(NULL);
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDevice* defaultDevice = NULL;
    IAudioEndpointVolume* endpointVolume = NULL;

    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);
    deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&endpointVolume);

    endpointVolume->SetMute(state, NULL); 

    endpointVolume->Release();
    defaultDevice->Release();
    deviceEnumerator->Release();
    CoUninitialize();
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
            GamePacket pkt;
            pkt << static_cast<int>(PacketType::GameStart);
            m_network.Send(pkt);
        }
        else if (m_state == ClientState::Game)
        {
            HandleGameInput(keyEvent->code);
        }
        else if (m_state == ClientState::Result && keyEvent->code == sf::Keyboard::Key::Enter)
        {
            GamePacket pkt;
            pkt << static_cast<int>(PacketType::GameStart);
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
            
            if (m_network.Connect(m_ipInput))
            {
                PlaySound(m_soundJoin);
                m_state = ClientState::Login;
                m_serverMessage = "Connecte ! Entrez votre pseudo.";
                m_messageColor = sf::Color::Green;
            }
            else
            {
                PlaySound(m_soundLose);
                m_serverMessage = "Erreur: Impossible de joindre " + m_ipInput;
                m_messageColor = sf::Color::Red;
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
            GamePacket loginPkt;
            loginPkt << static_cast<int>(PacketType::Connect) << m_pseudoInput;
            m_network.Send(loginPkt);

            m_state = ClientState::Lobby;
            m_serverMessage = "Bienvenue " + m_pseudoInput + " !";
            m_playerNames.push_back(m_pseudoInput); // Add self to list
            PlaySound(m_soundJoin);
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
        GamePacket pkt;
        pkt << static_cast<int>(PacketType::GameData) << m_currentNumberChoice;
        m_network.Send(pkt);
        
        m_serverMessage = "Envoi...";
        m_messageColor = sf::Color::Yellow;
        PlaySound(m_soundSelect);
    }
}


// --- GESTION DE L'UI

// --- UI HELPERS ---

void GameClient::DrawBackground()
{
    // Gradient or pattern could be added here
}

void GameClient::DrawPanel(float x, float y, float w, float h, sf::Color color)
{
    sf::RectangleShape panel({w, h});
    panel.setPosition({x, y});
    panel.setFillColor(color);
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(sf::Color(60, 60, 80));
    m_window.draw(panel);
}

void GameClient::DrawButton(const std::string& textStr, float y, bool active)
{
    sf::RectangleShape btn({300.f, 40.f});
    btn.setOrigin({150.f, 20.f});
    btn.setPosition({400.f, y});
    btn.setFillColor(active ? sf::Color(0, 200, 255) : sf::Color(60, 60, 70));
    
    m_window.draw(btn);

    sf::Text text(m_font);
    text.setString(textStr);
    text.setFillColor(active ? sf::Color::Black : sf::Color(200, 200, 200));
    CenterText(text, y - 10.f, 18);
    m_window.draw(text);
}

void GameClient::DrawInputBox(const std::string& label, const std::string& value, float y, bool active)
{
    sf::Text lbl(m_font);
    lbl.setString(label);
    lbl.setFillColor(sf::Color(150, 150, 150));
    CenterText(lbl, y - 50.f, 16);
    m_window.draw(lbl);

    sf::RectangleShape box({300.f, 40.f});
    box.setOrigin({150.f, 20.f});
    box.setPosition({400.f, y});
    box.setFillColor(sf::Color(30, 30, 45));
    box.setOutlineThickness(active ? 2.f : 1.f);
    box.setOutlineColor(active ? sf::Color(0, 200, 255) : sf::Color(100, 100, 100));
    m_window.draw(box);

    sf::Text val(m_font);
    val.setString(value + (active ? "_" : ""));
    val.setFillColor(sf::Color::White);
    CenterText(val, y - 12.f, 20);
    m_window.draw(val);
}

// --- ECRANS REFONDUS ---

void GameClient::DrawIpConfig()
{
    sf::Text title(m_font);
    title.setString("CONFIGURATION");
    title.setFillColor(sf::Color(0, 200, 255));
    CenterText(title, 80, 40);
    m_window.draw(title);

    DrawInputBox("ADRESSE IP DU SERVEUR", m_ipInput, 250.f, true);
    
    DrawButton("SE CONNECTER (ENTREE)", 350.f, !m_ipInput.empty());

    sf::Text errorMsg(m_font);
    errorMsg.setString(m_serverMessage);
    errorMsg.setFillColor(m_messageColor);
    CenterText(errorMsg, 450, 18);
    m_window.draw(errorMsg);
}

void GameClient::DrawLogin()
{
    sf::Text title(m_font);
    title.setString("IDENTIFICATION");
    title.setFillColor(sf::Color(0, 200, 255));
    CenterText(title, 80, 40);
    m_window.draw(title);

    DrawInputBox("CHOISISSEZ VOTRE PSEUDO", m_pseudoInput, 250.f, true);

    DrawButton("VALIDER (ENTREE)", 350.f, !m_pseudoInput.empty());
}

void GameClient::DrawLobby()
{
    // Players Panel
    DrawPanel(20.f, 20.f, 200.f, 560.f, sf::Color(30, 30, 40));
    
    sf::Text listTitle(m_font);
    listTitle.setString("JOUEURS");
    listTitle.setFillColor(sf::Color(0, 200, 255));
    listTitle.setCharacterSize(20);
    listTitle.setPosition({40.f, 40.f});
    listTitle.setStyle(sf::Text::Bold);
    m_window.draw(listTitle);

    float y = 80.f;
    for (const auto& name : m_playerNames)
    {
        sf::Text pName(m_font);
        pName.setString(name);
        pName.setFillColor(name == m_pseudoInput ? sf::Color(0, 255, 100) : sf::Color::White);
        pName.setCharacterSize(16);
        pName.setPosition({40.f, y});
        m_window.draw(pName);
        y += 25.f;
    }

    // Main Area
    sf::Text title(m_font);
    title.setString("SALLE D'ATTENTE");
    title.setFillColor(sf::Color::White);
    CenterText(title, 50, 40);
    m_window.draw(title);

    sf::Text status(m_font);
    status.setString(m_serverMessage);
    status.setFillColor(sf::Color(200, 200, 200));
    CenterText(status, 150, 20);
    m_window.draw(status);

    DrawButton("LANCER (TOUCHE S)", 250.f, true);
}

void GameClient::DrawGame()
{
    // Info Panel
    DrawPanel(250.f, 20.f, 300.f, 100.f, sf::Color(30, 30, 40));
    
    sf::Text title(m_font);
    title.setString("DEVINE LE NOMBRE");
    title.setFillColor(sf::Color(0, 200, 255));
    CenterText(title, 40, 24);
    m_window.draw(title);

    sf::Text result(m_font);
    result.setString(m_serverMessage);
    result.setFillColor(m_messageColor);
    CenterText(result, 80, 20);
    m_window.draw(result);

    // Number Display
    sf::CircleShape circle(80.f);
    circle.setFillColor(sf::Color(30, 30, 45));
    circle.setOutlineThickness(4.f);
    circle.setOutlineColor(sf::Color(0, 200, 255));
    circle.setOrigin({80.f, 80.f});
    circle.setPosition({400.f, 250.f});
    m_window.draw(circle);

    sf::Text nb(m_font);
    nb.setString(std::to_string(m_currentNumberChoice));
    nb.setFillColor(sf::Color::White);
    CenterText(nb, 215, 60);
    m_window.draw(nb);

    // Controls
    DrawButton("VALIDER (ENTREE)", 450.f, true);
    
    sf::Text hint(m_font);
    hint.setString("^ HAUT / BAS v");
    hint.setFillColor(sf::Color(100, 100, 100));
    CenterText(hint, 380, 16);
    m_window.draw(hint);
}

void GameClient::DrawResult()
{
     sf::Text title(m_font);
    title.setString("RESULTAT");
    title.setFillColor(sf::Color(255, 50, 100));
    CenterText(title, 80, 50);
    m_window.draw(title);

    sf::Text winner(m_font);
    winner.setString("Vainqueur : " + m_winnerName);
    winner.setFillColor(sf::Color::White);
    CenterText(winner, 200, 30);
    m_window.draw(winner);

    DrawButton("REJOUER (ENTREE)", 400.f, true);
}

void GameClient::CenterText(sf::Text& text, float y, int fontSize)
{
    text.setCharacterSize(fontSize);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition({400.f - (bounds.size.x / 2.f), y});
}