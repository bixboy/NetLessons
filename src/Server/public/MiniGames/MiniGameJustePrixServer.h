#pragma once
#include "IServerMiniGame.h"


class MiniGameJustePrixServer : public IServerMiniGame
{
public:
    void Start(GameServer* server, MiniGameSystem* system) override;
    void Update(float dt) override;
    bool HandlePacket(OpCode opcode, GamePacket& packet, ENetPeer* sender) override;
    
private:
    GameServer* m_server = nullptr;
    MiniGameSystem* m_system = nullptr;
    int m_mysteryNumber = 0;
};
