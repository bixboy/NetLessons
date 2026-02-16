#pragma once
#include "NetworkCommon.h"
#include "PacketSystem.h"
#include <functional>
#include <map>
#include <iostream>


class NetworkClient
{
public:
    using PacketHandler = std::function<void(GamePacket&)>;

    NetworkClient();
    ~NetworkClient();

    // Connexion
    bool Connect(const std::string& ip, int port = PORT);
    void Disconnect();
    
    // Packets
    void Send(GamePacket& packet);
    void Send(const IPacket& packet);
    void PollEvents();
    void OnPacket(OpCode type, PacketHandler handler);
    
    // Disconnection
    bool IsConnected() const { return m_isConnected; }
    void SetOnDisconnect(std::function<void(const std::string&)> handler) { m_onDisconnect = handler; }

private:
    // Handlers
    std::map<OpCode, PacketHandler> m_handlers;
    std::function<void(const std::string&)> m_onDisconnect;
    
    // ENet data
    ENetHost* m_client = nullptr;
    ENetPeer* m_peer = nullptr;
    bool m_isConnected = false;
};
