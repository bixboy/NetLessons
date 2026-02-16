#pragma once
#include <random>
#include <string>
#include <vector>
#include "IServerSystem.h"
#include "PacketSystem.h"


class MiniGameSystem : public IServerSystem
{
public:
    MiniGameSystem();
    void Init(GameServer* server) override;
    void Update(float dt) override;
    void OnPlayerDisconnect(PlayerInfo* player) override;
    void OnPlayerConnect(PlayerInfo* player) override;

    // Called by MovementSystem via GameServer::OnPushHit
    void HandlePushHit(const std::string& pusher, const std::string& target);

private:
    void StartHotPotato(GameServer* server);
    void EndGame(const std::string& winnerName);
    void SetPlayerState(PlayerInfo& player, EPlayerState newState);
    void BroadcastBombState();
    void EliminateBombHolder();
    void PickRandomBombHolder();

    GameServer* m_server = nullptr;

    // Shared
    bool m_gameRunning = false;
    uint8_t m_activeGameID = 0;

    // Juste Prix
    int m_mysteryNumber = 0;

    // Hot Potato
    std::string m_bombHolder;
    float m_bombTimer = 0.f;
    float m_bombBroadcastTimer = 0.f;
    std::vector<std::string> m_alivePlayers;

    // RNG
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_dist;
};
