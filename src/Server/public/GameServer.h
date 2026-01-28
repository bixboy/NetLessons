#include "NetworkServer.h"

struct PlayerInfo
{
    sockaddr_in address;
    std::string pseudo;
    std::chrono::steady_clock::time_point lastPacketTime;
};

class GameServer
{
public:
    GameServer();
    ~GameServer();

    bool Initialize();
    void Run();

private:
    void HandlePacket(GamePacket& pkt, const sockaddr_in& sender);
    
    PlayerInfo* GetPlayerByAddr(const sockaddr_in& addr);
    
    void RemovePlayer(const sockaddr_in& addr);
    void Broadcast(GamePacket& pkt, const sockaddr_in* senderToIgnore = nullptr);
    void SendTo(const sockaddr_in& target, GamePacket& pkt);

    NetworkServer m_network;
    
    std::vector<PlayerInfo> m_players;

    bool m_gameRunning;
    int m_mysteryNumber;
};