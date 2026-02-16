#pragma once
#include <enet/enet.h>
#include <vector>
#include <string>
#include <functional>
#include <map>

#include "PacketSystem.h"

class NetworkServer
{
public:
    NetworkServer();
    ~NetworkServer();

    bool Start(unsigned short port);
    void Stop();

    void Broadcast(const GamePacket& packet, ENetPeer* excluded = nullptr);
    void Broadcast(const IPacket& packet, ENetPeer* excluded = nullptr);
    void SendTo(const GamePacket& packet, ENetPeer* peer);
    void SendTo(const IPacket& packet, ENetPeer* peer);
    void PollEvents();

    using PacketHandler = std::function<void(GamePacket&, ENetPeer*)>;
    void OnPacket(OpCode type, PacketHandler handler);

    // Callbacks système de haut niveau
    std::function<void(ENetPeer*)> OnConnect;
    std::function<void(ENetPeer*)> OnDisconnect;

private:
    ENetHost* m_host = nullptr;
    std::map<OpCode, PacketHandler> m_handlers;
};


