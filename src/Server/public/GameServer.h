#pragma once
#include <string>
#include <vector>
#include "NetworkCommon.h"


struct PlayerInfo
{
    SOCKET socket;
    std::string pseudo;
};

class GameServer
{
public:
    GameServer();
    ~GameServer();

    bool Initialize();
    void Run();

private:
    void HandleNewConnection();
    void HandleClientMessage(SOCKET sock);
    void RemoveClient(SOCKET sock);
    void Broadcast(Packet& pkt, SOCKET senderToIgnore = INVALID_SOCKET);
    void SendTo(SOCKET sock, Packet& pkt);

    SOCKET m_listener;
    fd_set m_masterSet;
    std::vector<PlayerInfo> m_players;

    bool m_gameRunning;
    int m_mysteryNumber;
};