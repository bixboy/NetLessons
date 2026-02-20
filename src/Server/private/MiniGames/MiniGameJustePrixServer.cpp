#include "MiniGames/MiniGameJustePrixServer.h"
#include "Core/GameServer.h"
#include "Systems/MiniGameSystem.h"
#include <iostream>

void MiniGameJustePrixServer::Start(GameServer* server, MiniGameSystem* system)
{
    m_server = server;
    m_system = system;
    
    m_mysteryNumber = m_system->m_dist(m_system->m_rng);
    std::cout << "[JUSTE PRIX] Mystere Genere: " << m_mysteryNumber << std::endl;

    PacketGameStart startPkt;
    startPkt.GameID = 0;
    m_server->Broadcast(startPkt);

    for (auto& [peer, info] : m_server->GetPlayers())
    {
        if (info.playerState == EPlayerState::Lobby)
        {
            m_system->SetPlayerState(info, EPlayerState::Playing);
        }
    }
}

void MiniGameJustePrixServer::Update(float dt)
{
}

bool MiniGameJustePrixServer::HandlePacket(OpCode opcode, GamePacket& packet, ENetPeer* sender)
{
    if (opcode == OpCode::GameData)
    {
        PacketGameData pkt;
        pkt.Deserialize(packet);
        
        PlayerInfo* player = m_server->GetPlayerByPeer(sender);
        if (player) 
            player->lastPacketTime = std::chrono::steady_clock::now();

        int guess = pkt.Value;
        
        if (guess < 0 || guess > 99)
            return true;
        
        PacketGameData response;
        
        if (guess < m_mysteryNumber)
        {
            response.Value = static_cast<int>(EGameDataType::JustePrixHintUp);
            m_server->SendTo(sender, response);
        }
        else if (guess > m_mysteryNumber)
        {
            response.Value = static_cast<int>(EGameDataType::JustePrixHintDown);
            m_server->SendTo(sender, response);
        }
        else
        {
            // WIN
            m_system->EndGame(player ? player->pseudo : "Unknown");
        }
        return true;
    }
    return false;
}
