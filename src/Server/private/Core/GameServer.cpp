#include "Core/GameServer.h"
#include "Systems/AuthenticationSystem.h"
#include "Systems/ChatSystem.h"
#include "Systems/MiniGameSystem.h"

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
        
        for (auto& sys : m_systems)
        {
            sys->Update(dt);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}


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
        
        bool wasAdmin = it->isAdmin;
        m_players.erase(it);

        if (wasAdmin && !m_players.empty())
        {
            m_players[0].isAdmin = true;
            std::cout << "Nouveau ADMIN designe : " << m_players[0].pseudo << std::endl;

            PacketChat adminMsg;
            adminMsg.Sender = "SYSTEM";
            adminMsg.Message = m_players[0].pseudo + " est desormais l'ADMIN.";
            Broadcast(adminMsg);
        }
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

void GameServer::HandlePacket(GamePacket& pkt, const sockaddr_in& sender)
{
}