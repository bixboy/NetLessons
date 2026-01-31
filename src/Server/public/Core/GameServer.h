#include "../Network/NetworkServer.h"
#include "CommandManager.h"

struct PlayerInfo
{
    sockaddr_in address;
    std::string pseudo;
    std::chrono::steady_clock::time_point lastPacketTime;
};

#include <memory>
#include <vector>
#include "../Systems/IServerSystem.h"

class GameServer
{
public:
    GameServer();
    ~GameServer();

    bool Initialize();
    void Run();

    // Accessors for Systems
    NetworkServer& GetNetwork() { return m_network; }
    CommandManager& GetCommandManager() { return m_commandManager; }
    std::vector<PlayerInfo>& GetPlayers() { return m_players; }

    // System Management
    template <typename T>
    T* AddSystem()
    {
        auto system = std::make_unique<T>();
        T* systemPtr = system.get();
        m_systems.push_back(std::move(system));
        return systemPtr;
    }

    void Broadcast(const IPacket& pkt, const sockaddr_in* senderToIgnore = nullptr);
    void SendTo(const sockaddr_in& target, const IPacket& pkt);
    
    // Player Management Helpers
    PlayerInfo* GetPlayerByAddr(const sockaddr_in& addr);
    void RemovePlayer(const sockaddr_in& addr);

private:
    void HandlePacket(GamePacket& pkt, const sockaddr_in& sender);
    
    NetworkServer m_network;
    CommandManager m_commandManager;
    
    std::vector<PlayerInfo> m_players;

    std::vector<std::unique_ptr<IServerSystem>> m_systems;
};