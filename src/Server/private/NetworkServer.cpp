#include "NetworkServer.h"

#include "NetworkCommon.h"
#include "PacketSystem.h"

#include <iostream>


NetworkServer::NetworkServer() : m_socket(INVALID_SOCKET), m_isRunning(false)
{
}

NetworkServer::~NetworkServer()
{
    Stop();
}

bool NetworkServer::Start(unsigned short port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return false;
    }

    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(port);
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_socket, reinterpret_cast<sockaddr*>(&m_serverAddr), sizeof(m_serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(m_socket);
        WSACleanup();
        return false;
    }

    m_isRunning = true;
    m_receiveThread = std::thread(&NetworkServer::ReceiveLoop, this);

    std::cout << "NetworkServer started on port " << port << "\n";
    return true;
}

void NetworkServer::Stop()
{
    m_isRunning = false;
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    
    if (m_receiveThread.joinable())
    {
        m_receiveThread.join();
    }
    
    WSACleanup();
}

void NetworkServer::SendTo(const GamePacket& packet, const sockaddr_in& address)
{
    if (m_socket == INVALID_SOCKET)
        return;

    int sentBytes = sendto(m_socket, packet.Data(), packet.Size(), 0, reinterpret_cast<const sockaddr*>(&address), sizeof(address));

    if (sentBytes == SOCKET_ERROR)
    {
        std::cerr << "SendTo failed: " << WSAGetLastError() << "\n";
    }
}

void NetworkServer::SendTo(const IPacket& packet, const sockaddr_in& address)
{
    GamePacket rawPacket;
    packet.Serialize(rawPacket);
    SendTo(rawPacket, address);
}

void NetworkServer::ReceiveLoop()
{
    char buffer[MAX_PACKET_SIZE];
    
    sockaddr_in sender;
    int senderLen = sizeof(sender);

    while (m_isRunning)
    {
        int bytes = recvfrom(m_socket, buffer, MAX_PACKET_SIZE, 0, reinterpret_cast<sockaddr*>(&sender), &senderLen);
        if (bytes > 0)
        {
            ReceivedPacket rx;
            rx.packet = GamePacket(buffer, bytes);
            rx.sender = sender;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_packetQueue.push(rx);
        }
        else
        {
            if (m_isRunning)
            {
                std::cerr << "Recvfrom error: " << WSAGetLastError() << "\n";
            }
        }
    }
}

void NetworkServer::PollEvents()
{
    std::queue<ReceivedPacket> tempQueue;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::swap(tempQueue, m_packetQueue);
    }

    while (!tempQueue.empty())
    {
        ReceivedPacket& p = tempQueue.front();

        int typeInt = 0;
        p.packet >> typeInt;
        OpCode type = static_cast<OpCode>(typeInt);

        auto it = m_handlers.find(type);
        if (it != m_handlers.end())
        {
            it->second(p.packet, p.sender);
        }
        
        tempQueue.pop();
    }
}

void NetworkServer::OnPacket(OpCode type, PacketHandler handler)
{
    m_handlers[type] = handler;
}
