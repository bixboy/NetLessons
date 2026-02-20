#include "Core/GameServer.h"
#include "Systems/AuthenticationSystem.h"
#include "Systems/ChatSystem.h"
#include "Systems/MiniGameSystem.h"
#include "Systems/MovementSystem.h"

#include "NetworkCommon.h"
#include "PacketSystem.h"

#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>


GameServer::GameServer() : m_commandManager(this)
{
}

GameServer::~GameServer()
{
}

bool GameServer::Initialize()
{
    if (!m_network.Start(PORT))
        return false;
        
    m_network.OnDisconnect = [this](ENetPeer* peer)
    {
        RemovePlayer(peer);
    };

    AddSystem<AuthenticationSystem>()->Init(this);
    AddSystem<ChatSystem>()->Init(this);
    AddSystem<MiniGameSystem>()->Init(this);
    AddSystem<MovementSystem>()->Init(this);
    
    return true;
}

void GameServer::Run()
{
    std::cout << "Server loop running..." << std::endl;
    
    auto lastTime = std::chrono::steady_clock::now();

    while (true)
    {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        m_network.PollEvents();
        
        for (auto& sys : m_systems)
        {
            sys->Update(dt);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


PlayerInfo* GameServer::GetPlayerByPeer(ENetPeer* peer)
{
    auto it = m_players.find(peer);
    if (it != m_players.end())
        return &it->second;
    
    return nullptr;
}

void GameServer::RemovePlayer(ENetPeer* peer)
{
    auto it = m_players.find(peer);
    if (it != m_players.end())
    {
        std::cout << "Deconnexion : " << it->second.pseudo << std::endl;
        
        PacketConnectionState leavePkt;
        leavePkt.IsConnected = false;
        leavePkt.Pseudo = it->second.pseudo;
        Broadcast(leavePkt, peer);
        
        bool wasAdmin = it->second.isAdmin;
        
        for (auto& sys : m_systems)
            sys->OnPlayerDisconnect(&it->second);
        
        m_players.erase(it);

        if (wasAdmin && !m_players.empty())
        {
            auto& newAdmin = m_players.begin()->second;
            newAdmin.isAdmin = true;
            std::cout << "Nouveau ADMIN designe : " << newAdmin.pseudo << std::endl;

            PacketChat adminMsg;
            adminMsg.Sender = "SYSTEM";
            adminMsg.Message = newAdmin.pseudo + " est desormais l'ADMIN.";
            Broadcast(adminMsg);
        }
    }
}

void GameServer::Broadcast(const IPacket& pkt, ENetPeer* senderToIgnore)
{
    m_network.Broadcast(pkt, senderToIgnore);
}

void GameServer::SendTo(ENetPeer* target, const IPacket& pkt)
{
    m_network.SendTo(pkt, target);
}

void GameServer::NotifyPlayerConnect(PlayerInfo* player)
{
    for (auto& sys : m_systems)
        sys->OnPlayerConnect(player);
}