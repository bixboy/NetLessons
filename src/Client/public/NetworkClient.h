#pragma once
#include "../../CommonNet/NetworkCommon.h"
#include <functional>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
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
    void PollEvents();
    void OnPacket(PacketType type, PacketHandler handler);
    
    // Disconnection
    bool IsConnected() const { return m_isConnected; }
    void SetOnDisconnect(std::function<void(const std::string&)> handler) { m_onDisconnect = handler; }

private:
    void ReceiveLoop();
    void PushPacket(GamePacket pkt);

    // Socket data
    SOCKET m_socket;
    sockaddr_in m_serverAddr;
    int m_serverAddrLen;
    std::atomic<bool> m_isConnected;
    std::atomic<bool> m_shouldRun;

    // Threading
    std::thread m_receiveThread;
    
    // Thread-Safe Queue
    std::queue<GamePacket> m_packetQueue;
    std::mutex m_queueMutex;

    // Handlers
    std::map<PacketType, PacketHandler> m_handlers;
    std::function<void(const std::string&)> m_onDisconnect;
};
