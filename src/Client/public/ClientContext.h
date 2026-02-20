#pragma once
#include "Network/NetworkClient.h"
#include "Managers/SoundManager.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>


enum class ClientState;
class AvatarManager;

enum class ClientState
{
    IpConfig,
    Login,
    Lobby,
    Game,
    Result
};


struct MiniGameInfo
{
    uint8_t ID;
    std::string Name;
    std::string Description;
};


struct ClientContext
{
    // --- State Machine ---
    ClientState State = ClientState::IpConfig;

    // --- Network ---
    NetworkClient Network;
    std::string IpInput = "127.0.0.1";
    sf::Clock PingClock;

    // --- Player ---
    std::string PseudoInput;
    std::vector<std::string> PlayerNames;
    std::map<std::string, sf::Color> PlayerColors;

    // --- Game Data ---
    std::string ServerMessage = "Veuillez entrer l'IP";
    sf::Color MessageColor = sf::Color::White;
    int CurrentNumberChoice = 50;
    int GuessAttempts = 0;
    int LastHint = 0;
    std::string WinnerName;
    std::vector<MiniGameInfo> AvailableGames;

    // --- Hot Potato ---
    int ActiveGameID = 0;
    std::string BombHolder;
    float BombTimer = 0.f;
    std::vector<std::string> AlivePlayers;
    bool IsGameRunning = false;
    float RespawnTimer = 0.f;
    bool IsRedLight = false;
    bool IsIceMode = false;
    int TrollLight = 0;
    EPlayerState LocalPlayerState = EPlayerState::Lobby;
    bool IsSpectator() const { return LocalPlayerState == EPlayerState::Spectating; }

    // --- Managers ---
    SoundManager Sound;
    AvatarManager* Avatars = nullptr;

    // --- Window / Rendering ---
    sf::RenderWindow Window;
    sf::RenderTarget* Target = &Window;
    sf::Font Font;
    sf::Vector2u WindowSize;
    float CenterX = 0.f;

    // --- Animation ---
    sf::Clock AnimClock;
    float PulseValue = 0.f;

    // --- Helpers ---
    static sf::Color GetColorFromID(uint8_t id)
    {
        static const sf::Color colors[] = {
            sf::Color(255, 100, 100),
            sf::Color(100, 255, 100),
            sf::Color(100, 100, 255),
            sf::Color(255, 255, 100),
            sf::Color(255, 100, 255),
            sf::Color(100, 255, 255),
            sf::Color(255, 165, 0),
            sf::Color(150, 100, 255)
        };
        
        return (id < 8) ? colors[id] : sf::Color::White;
    }
};
