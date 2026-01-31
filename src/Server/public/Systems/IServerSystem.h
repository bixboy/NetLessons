#pragma once

class GameServer;
struct PlayerInfo;

class IServerSystem
{
public:
    virtual ~IServerSystem() = default;

    virtual void Init(GameServer* server) = 0;
    virtual void Update(float dt) {}
    virtual void OnPlayerDisconnect(PlayerInfo* player) {}
};
