#pragma once
#include "NetworkClient.h"
#include "ChatBox.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <atomic>
#include <mutex>

enum class ClientState
{
    IpConfig,
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

    void Run();

private:
    // --- Réseau ---
    void SetupNetworkHandlers();
    
    // --- Inputs ---
    void HandleInput(const sf::Event& event);
    void HandleIpInput(const sf::Event& event);
    void HandleLoginInput(const sf::Event& event);
    void HandleGameInput(sf::Keyboard::Key key);
    
    
    // --- Affichage ---
    void DrawIpConfig();
    void DrawLogin();
    void DrawLobby();
    void DrawGame();
    void DrawResult();

    void CenterText(sf::Text& text, float y, int fontSize);
    
    // UI Helpers
    void DrawBackground();
    void DrawPanel(float x, float y, float w, float h, sf::Color color, float cornerRadius = 0.f);
    void DrawButton(const std::string& text, float y, bool active = false, bool hovered = false);
    void DrawInputBox(const std::string& label, const std::string& value, float y, bool active = true);
    void UpdateLayout(); // Appelé lors du resize

    // --- AUDIO ---
    void InitSounds();
    void PlaySound(sf::Sound& sound);

    void SetWindowVolume(float volumeLevel);
    void SetMuteState(bool state);

    // Réseau
    NetworkClient m_network;
    std::string m_ipInput;
    sf::Clock m_pingClock;

    // Chat
    ChatBox m_chat;
    
    // Graphiques
    sf::RenderWindow m_window;
    sf::Font m_font;
    sf::Vector2u m_windowSize;
    float m_centerX;

    ClientState m_state;
    
    // Animation
    sf::Clock m_animClock;
    float m_pulseValue;

    // --- Game Data ---
    std::string m_pseudoInput;
    std::string m_serverMessage;
    sf::Color m_messageColor;
    int m_currentNumberChoice;
    std::string m_winnerName;
    std::vector<std::string> m_playerNames;

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