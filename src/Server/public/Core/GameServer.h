#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <string>
#include <functional>

#include "Systems/IServerSystem.h"
#include "NetworkServer.h"
#include "CommandManager.h"
#include "PacketSystem.h"

class CommandManager;


struct PlayerInfo
{
    ENetPeer* peer = nullptr;
    std::string pseudo = "";
    std::chrono::steady_clock::time_point lastPacketTime = {};
    bool isAdmin = false;
    uint8_t colorID = 0;
    EPlayerState playerState = EPlayerState::Lobby;

    // Movement
    float posX = 0.5f;
    float posY = 0.5f;
    int8_t inputDirX = 0;
    int8_t inputDirY = 0;
    bool requestPush = false;
    
    // Physics & PvP
    float velocityX = 0.f;
    float velocityY = 0.f;
    float pushCooldown = 0.f;
    
    // Wall Pin Death Logic
    int wallPinCount = 0;
    float lastPinTime = 0.f;
    float respawnTimer = 0.f;
};

class GameServer
{
public:
    GameServer();
    ~GameServer();

    bool Initialize(int port);
    void Run();

    NetworkServer& GetNetwork() { return m_network; }
    CommandManager& GetCommandManager() { return m_commandManager; }
    std::unordered_map<ENetPeer*, PlayerInfo>& GetPlayers() { return m_players; }

    template <typename T>
    T* AddSystem()
    {
        auto system = std::make_unique<T>();
        T* systemPtr = system.get();
        m_systems.push_back(std::move(system));
        return systemPtr;
    }

    void Broadcast(const IPacket& pkt, ENetPeer* senderToIgnore = nullptr);
    void SendTo(ENetPeer* target, const IPacket& pkt);
    
    PlayerInfo* GetPlayerByPeer(ENetPeer* peer);
    void RemovePlayer(ENetPeer* peer);
    void NotifyPlayerConnect(PlayerInfo* player);

    std::function<void(const std::string& pusher, const std::string& target)> OnPushHit;

private:
    NetworkServer m_network;
    CommandManager m_commandManager;
    
    std::unordered_map<ENetPeer*, PlayerInfo> m_players;

    std::vector<std::unique_ptr<IServerSystem>> m_systems;
};