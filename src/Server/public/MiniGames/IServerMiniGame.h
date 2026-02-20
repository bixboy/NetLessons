#pragma once
#include <string>
#include "PacketSystem.h"

class GameServer;
struct PlayerInfo;
namespace ENet { struct Peer; }

class MiniGameSystem;

class IServerMiniGame
{
public:
    virtual ~IServerMiniGame() = default;

    virtual void Start(GameServer* server, MiniGameSystem* system) = 0;
    virtual void Update(float dt) = 0;
    virtual void OnPlayerConnect(PlayerInfo* player) {}
    virtual void OnPlayerDisconnect(PlayerInfo* player) {}
    virtual void OnPush(const std::string& pusher, const std::string& target) {}
    
    virtual bool IsIceMode() const { return false; }

    virtual bool HandlePacket(OpCode opcode, GamePacket& packet, ENetPeer* sender) { return false; }
    
    virtual void End() {}
};
