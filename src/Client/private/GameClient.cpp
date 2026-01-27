#include "../public/GameClient.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <mmdeviceapi.h>
#include <endpointvolume.h>


GameClient::GameClient() 
    : m_socket(INVALID_SOCKET), m_isRunning(false), 
         m_state(ClientState::Login),
        m_currentNumberChoice(50), 
        m_serverMessage("En attente du serveur..."), 
        m_messageColor(sf::Color::White),
        m_soundSelect(m_bufferSelect),
        m_soundWin(m_bufferWin),
        m_soundLose(m_bufferLose),
        m_soundJoin(m_bufferJoin),
        m_soundLeave(m_bufferLeave)
{
}

GameClient::~GameClient()
{
    PlaySound(m_soundLeave);
    m_isRunning = false;
    closesocket(m_socket);
    WSACleanup();
}

bool GameClient::Connect(const std::string& ip)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    inet_pton(AF_INET, ip.c_str(), &hint.sin_addr);

    if (connect(m_socket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
        return false;
    
    m_isRunning = true;
    return true;
}

void GameClient::Run()
{
    m_window.create(sf::VideoMode({800, 600}), "Le Juste Prix - Network");
    m_window.setFramerateLimit(60);

    if (!m_font.openFromFile("assets/arial.ttf"))
    {
        std::cerr << "ERREUR: arial.ttf manquant !" << std::endl;
    }

    InitSounds();

    std::thread t(&GameClient::ReceiveLoop, this);
    t.detach();

    // --- BOUCLE PRINCIPALE ---
    while (m_window.isOpen() && m_isRunning)
    {
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

        m_window.clear(sf::Color(25, 25, 35));

        switch (m_state)
        {
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

    // 7. On ferme la librairie COM
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
    if (m_state == ClientState::Login)
    {
        HandleLoginInput(event);
    }
    else if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (m_state == ClientState::Lobby && keyEvent->code == sf::Keyboard::Key::S)
        {
            Packet pkt{ 2, 0, "" };
            SendPacket(pkt);
        }
        else if (m_state == ClientState::Game)
        {
            HandleGameInput(keyEvent->code);
        }
        else if (m_state == ClientState::Result && keyEvent->code == sf::Keyboard::Key::Enter)
        {
            Packet pkt{ 2, 0, "" };
            SendPacket(pkt);
            m_serverMessage = "En attente du serveur...";
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
            Packet loginPkt{ 3, 0, "" };
            strncpy(loginPkt.text, m_pseudoInput.c_str(), 31);
            SendPacket(loginPkt);

            m_state = ClientState::Lobby;
            m_serverMessage = "Bienvenue " + m_pseudoInput + " !";
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
        Packet pkt{ 1, m_currentNumberChoice, "" };
        SendPacket(pkt);
        m_serverMessage = "Envoi...";
        m_messageColor = sf::Color::Yellow;
        PlaySound(m_soundSelect);
    }
}


// --- GESTION DE L'UI

void GameClient::DrawLogin()
{
    sf::Text title(m_font);
    title.setString("CONNEXION");
    title.setFillColor(sf::Color::Cyan);
    CenterText(title, 100, 50);
    m_window.draw(title);

    sf::RectangleShape box({400.f, 50.f});
    box.setFillColor(sf::Color(50, 50, 60));
    box.setOutlineThickness(2);
    box.setOutlineColor(sf::Color::White);
    box.setOrigin({200.f, 25.f});
    box.setPosition({400.f, 300.f});
    m_window.draw(box);

    sf::Text pseudoText(m_font);
    pseudoText.setString(m_pseudoInput + "_");
    CenterText(pseudoText, 285, 30);
    m_window.draw(pseudoText);

    sf::Text help(m_font);
    help.setString("Entrez votre pseudo et appuyez sur ENTREE");
    help.setFillColor(sf::Color(150, 150, 150));
    CenterText(help, 400, 18);
    m_window.draw(help);
}

void GameClient::DrawLobby()
{
    sf::Text title(m_font);
    title.setString("LOBBY");
    title.setFillColor(sf::Color::Cyan);
    CenterText(title, 50, 40);
    m_window.draw(title);

    sf::Text msg(m_font);
    msg.setString(m_serverMessage);
    msg.setFillColor(sf::Color::White);
    CenterText(msg, 250, 24);
    m_window.draw(msg);

    sf::Text help(m_font);
    help.setString("Attente des joueurs...\nAppuyez sur 'S' pour lancer la partie");
    help.setFillColor(sf::Color::Yellow);
    CenterText(help, 500, 20);
    m_window.draw(help);
}

void GameClient::DrawGame()
{
    sf::Text nb(m_font);
    nb.setString(std::to_string(m_currentNumberChoice));
    nb.setFillColor(sf::Color::Cyan);
    CenterText(nb, 200, 120);
    m_window.draw(nb);

    sf::Text result(m_font);
    result.setString(m_serverMessage);
    result.setFillColor(m_messageColor);
    CenterText(result, 50, 40);
    m_window.draw(result);

    sf::Text controls(m_font);
    controls.setString("[HAUT/BAS] Choisir   [ENTREE] Valider");
    controls.setFillColor(sf::Color(100, 100, 100));
    CenterText(controls, 550, 18);
    m_window.draw(controls);
}

void GameClient::DrawResult()
{
    sf::Text title(m_font);
    title.setString("PARTIE TERMINEE");
    title.setFillColor(sf::Color::White);
    CenterText(title, 100, 50);
    m_window.draw(title);

    sf::Text winner(m_font);
    winner.setString("Vainqueur :\n" + m_winnerName);
    winner.setFillColor(sf::Color::Green);
    CenterText(winner, 250, 60);
    m_window.draw(winner);

    sf::Text info(m_font);
    info.setString("Appuyez sur ENTREE pour relancer la partie...");
    info.setFillColor(sf::Color::Yellow);
    CenterText(info, 500, 20);
    m_window.draw(info);
}

void GameClient::CenterText(sf::Text& text, float y, int fontSize)
{
    text.setCharacterSize(fontSize);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setPosition({400.f - (bounds.size.x / 2.f), y});
}

// --- RÉSEAU ---

void GameClient::ReceiveLoop()
{
    Packet pkt;
    while (m_isRunning)
    {
        int bytes = recv(m_socket, (char*)&pkt, BUFFER_SIZE, 0);
        if (bytes > 0)
        {
            if (pkt.data == 1)
            {
                m_serverMessage = "C'est PLUS (+)";
                m_messageColor = sf::Color::Red;
            }
            else if (pkt.data == 2)
            {
                m_serverMessage = "C'est MOINS (-)";
                m_messageColor = sf::Color::Magenta;
            } 
            // VICTOIRE
            else if (pkt.data == 3)
            {
                m_state = ClientState::Result;
                m_winnerName = "TOI (Bravo !)";

                SetMuteState(false);
                SetWindowVolume(100.f);
                PlaySound(m_soundWin);
            }
            // DÉFAITE
            else if (pkt.data == 4)
            {
                m_state = ClientState::Result;
                m_winnerName = std::string(pkt.text);
                PlaySound(m_soundLose);
            }
            // START GAME
            else if (pkt.data == 5)
            {
                m_state = ClientState::Game;
                m_serverMessage = "DEVINE LE NOMBRE !";
                m_messageColor = sf::Color::White;
                PlaySound(m_soundSelect);
            }
            // JOIN
            else if (pkt.type == 6)
            {
                m_serverMessage = std::string(pkt.text) + " a rejoint !";
                m_messageColor = sf::Color::Cyan;
                PlaySound(m_soundJoin);
            }
            // LEAVE
            else if (pkt.type == 7)
            {
                m_serverMessage = std::string(pkt.text) + " est parti.";
                m_messageColor = sf::Color::Yellow;
                PlaySound(m_soundLeave);
            }
        }
        else
        {
            m_isRunning = false;
        }
    }
}

void GameClient::SendPacket(Packet& pkt)
{
    send(m_socket, (char*)&pkt, BUFFER_SIZE, 0);
}