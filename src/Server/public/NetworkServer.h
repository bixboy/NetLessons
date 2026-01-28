#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include "../../CommonNet/NetworkCommon.h"

#pragma comment(lib, "ws2_32.lib")

class NetworkServer
{
public:
    NetworkServer();
    ~NetworkServer();

    bool Start(unsigned short port);
    void Stop();

    void SendTo(const GamePacket& packet, const sockaddr_in& address);
    void PollEvents();

    using PacketHandler = std::function<void(GamePacket&, const sockaddr_in&)>;
    void OnPacket(PacketType type, PacketHandler handler);

private:
    void ReceiveLoop();

    SOCKET m_socket;
    sockaddr_in m_serverAddr;
    std::atomic<bool> m_isRunning;
    std::thread m_receiveThread;

    struct ReceivedPacket
    {
        GamePacket packet;
        sockaddr_in sender;
    };

    std::mutex m_mutex;
    std::queue<ReceivedPacket> m_packetQueue;
    std::map<PacketType, PacketHandler> m_handlers;
};
