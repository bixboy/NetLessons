#pragma once
#include <random>
#include <string>
#include <vector>
#include "IServerSystem.h"
#include "PacketSystem.h"
#include <memory>
#include "MiniGames/IServerMiniGame.h"


class MiniGameSystem : public IServerSystem
{
public:
    static MiniGameSystem* s_instance;

    MiniGameSystem();
    virtual ~MiniGameSystem();
    
    void Init(GameServer* server) override;
    
    void Update(float dt) override;
    
    void OnPlayerDisconnect(PlayerInfo* player) override;
    void OnPlayerConnect(PlayerInfo* player) override;

    void HandlePushHit(const std::string& pusher, const std::string& target);

    void SetPlayerState(PlayerInfo& player, EPlayerState newState);
    void EndGame(const std::string& winnerName);

private:

    GameServer* m_server = nullptr;

    bool m_gameRunning = false;
    uint8_t m_activeGameID = 0;

    std::unique_ptr<IServerMiniGame> m_currentGame;

public:
    std::vector<std::string> m_alivePlayers;

    bool IsIceMode() const;

    // RNG
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_dist;
};
