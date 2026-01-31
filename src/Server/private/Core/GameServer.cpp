#include "../../public/Core/GameServer.h"
#include "../../public/Systems/AuthenticationSystem.h"
#include "../../public/Systems/ChatSystem.h"
#include "../../public/Systems/MiniGameSystem.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>

GameServer::GameServer() : m_commandManager(this)
{
}

GameServer::~GameServer()
{
}

bool GameServer::Initialize()
{
    if (!m_network.Start(PORT))
    {
        return false;
    }

    // --- SYSTEMS ---
    AddSystem<AuthenticationSystem>()->Init(this);
    AddSystem<ChatSystem>()->Init(this);
    AddSystem<MiniGameSystem>()->Init(this);
    
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
        
        // Update Systems
        for (auto& sys : m_systems)
        {
            sys->Update(dt);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// HandlePacket logic moved to NetworkServer and Systems

PlayerInfo* GameServer::GetPlayerByAddr(const sockaddr_in& addr)
{
    for (auto& p : m_players)
    {
        if (p.address.sin_addr.s_addr == addr.sin_addr.s_addr && p.address.sin_port == addr.sin_port)
        {
            return &p;
        }
    }
    
    return nullptr;
}

void GameServer::RemovePlayer(const sockaddr_in& addr)
{
    auto it = std::find_if(m_players.begin(), m_players.end(), [&](const PlayerInfo& p) 
        {
            return p.address.sin_addr.s_addr == addr.sin_addr.s_addr && p.address.sin_port == addr.sin_port;
        });

    if (it != m_players.end())
    {
        std::cout << "Deconnexion : " << it->pseudo << std::endl;
        
        PacketConnectionState leavePkt;
        leavePkt.IsConnected = false;
        leavePkt.Pseudo = it->pseudo;
        Broadcast(leavePkt, &addr);
        
        m_players.erase(it);
    }
}

void GameServer::Broadcast(const IPacket& pkt, const sockaddr_in* senderToIgnore)
{
    for (auto& p : m_players)
    {
        if (senderToIgnore != nullptr && p.address.sin_addr.s_addr == senderToIgnore->sin_addr.s_addr && p.address.sin_port == senderToIgnore->sin_port)
            continue;
        
        SendTo(p.address, pkt);
    }
}

void GameServer::SendTo(const sockaddr_in& target, const IPacket& pkt)
{
    m_network.SendTo(pkt, target);
}