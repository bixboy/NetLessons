#pragma once
#include "NetworkCommon.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <atomic>

enum class ClientState
{
    Login,
    Lobby,
    Game,
    Result
};

class GameClient
{
public:
    GameClient();
    ~GameClient();

    bool Connect(const std::string& ip);
    void Run();

private:
    // --- Réseau ---
    void ReceiveLoop();
    void SendPacket(Packet& pkt);

    // --- Inputs ---
    void HandleInput(const sf::Event& event);
    void HandleLoginInput(const sf::Event& event);
    void HandleGameInput(sf::Keyboard::Key key);

    // --- Affichage ---
    void DrawLogin();
    void DrawLobby();
    void DrawGame();
    void DrawResult();

    void CenterText(sf::Text& text, float y, int fontSize);

    // --- AUDIO ---
    void InitSounds();
    void PlaySound(sf::Sound& sound);

    void SetWindowVolume(float volumeLevel);
    void SetMuteState(bool state);

    // Variables Réseau
    SOCKET m_socket;
    std::atomic<bool> m_isRunning;
    
    // Variables Graphiques
    sf::RenderWindow m_window;
    sf::Font m_font;

    std::atomic<ClientState> m_state;

    // --- Game Data ---
    std::string m_pseudoInput;
    std::string m_serverMessage;
    sf::Color m_messageColor;
    int m_currentNumberChoice;
    std::string m_winnerName;

    // --- Sounds ---
    sf::SoundBuffer m_bufferSelect;
    sf::Sound m_soundSelect;

    sf::SoundBuffer m_bufferWin;
    sf::Sound m_soundWin;

    sf::SoundBuffer m_bufferLose;
    sf::Sound m_soundLose;

    sf::SoundBuffer m_bufferJoin;
    sf::Sound m_soundJoin;

    sf::SoundBuffer m_bufferLeave;
    sf::Sound m_soundLeave;
};