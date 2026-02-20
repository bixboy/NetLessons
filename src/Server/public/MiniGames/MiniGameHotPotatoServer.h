#pragma once
#include "IServerMiniGame.h"
#include <string>


class MiniGameHotPotatoServer : public IServerMiniGame
{
public:
    void Start(GameServer* server, MiniGameSystem* system) override;
    void Update(float dt) override;
    void OnPlayerDisconnect(PlayerInfo* player) override;
    void OnPush(const std::string& pusher, const std::string& target) override;

private:
    void PickRandomBombHolder();
    void EliminateBombHolder();
    void BroadcastBombState();

    GameServer* m_server = nullptr;
    MiniGameSystem* m_system = nullptr;

    std::string m_bombHolder;
    float m_bombTimer = 0.f;
    float m_passCooldownTimer = 0.f;
    float m_bombBroadcastTimer = 0.f;
};
