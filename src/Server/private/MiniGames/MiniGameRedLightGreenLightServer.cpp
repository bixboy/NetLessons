#include "MiniGames/MiniGameRedLightGreenLightServer.h"
#include "Core/GameServer.h"
#include "Systems/MiniGameSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>

void MiniGameRedLightGreenLightServer::Start(GameServer* server, MiniGameSystem* system)
{
    m_server = server;
    m_system = system;
    
    // Reset RLGL state
    m_rlgl = RedLightGreenLightState();
    m_rlgl.IsRedLight = true;
    m_rlgl.Timer = 3.0f;
    m_rlgl.GracePeriod = 2.0f;
    m_isIceMode = false;

    PacketGameStart startPkt;
    startPkt.GameID = 2;
    server->Broadcast(startPkt);

    m_system->m_alivePlayers.clear();
    for (auto& [peer, info] : server->GetPlayers())
    {
        if (info.playerState == EPlayerState::Lobby)
        {
            m_system->SetPlayerState(info, EPlayerState::Playing);
            m_system->m_alivePlayers.push_back(info.pseudo);

            info.posX = RedLightGreenLightState::START_LINE_X;
            info.posY = 0.5f + (m_system->m_dist(m_system->m_rng) % 100) / 500.0f;
            info.velocityX = 0.f;
            info.velocityY = 0.f;
        }
    }
}

void MiniGameRedLightGreenLightServer::Update(float dt)
{
    m_rlgl.Timer -= dt;
    if (m_rlgl.GracePeriod > 0.f)
        m_rlgl.GracePeriod -= dt;

    if (m_rlgl.Timer <= 0.f)
    {
        m_rlgl.IsRedLight = !m_rlgl.IsRedLight;

        if (m_rlgl.IsRedLight)
        {
            // SWITCH TO RED
            m_rlgl.Timer = 3.0f + (m_system->m_dist(m_system->m_rng) % 3);
            m_rlgl.GracePeriod = 0.4f;
            
            PacketGameData pkt;
            pkt.Value = static_cast<int>(EGameDataType::RedLight);
            m_server->Broadcast(pkt);

            // 1. Ice Mode Chance
            if ((m_system->m_dist(m_system->m_rng) % 5) == 0)
            {
                m_isIceMode = true;
                PacketGameData icePkt;
                icePkt.Value = static_cast<int>(EGameDataType::IceModeOn);
                m_server->Broadcast(icePkt);
            }
            else
            {
                if (m_isIceMode)
                {
                    m_isIceMode = false;
                    PacketGameData icePkt;
                    icePkt.Value = static_cast<int>(EGameDataType::IceModeOff);
                    m_server->Broadcast(icePkt);
                }
            }

            // 2. Troll Light Chance
            int roll = m_system->m_dist(m_system->m_rng) % 10;
            if (roll == 0)
            {
                 PacketGameData trollPkt;
                 trollPkt.Value = static_cast<int>(EGameDataType::LightPurple);
                 m_server->Broadcast(trollPkt);
                 m_rlgl.Timer = 1.0f;
                 m_rlgl.IsRedLight = false;
            }
            else if (roll == 1)
            {
                 PacketGameData trollPkt;
                 trollPkt.Value = static_cast<int>(EGameDataType::LightOrange);
                 m_server->Broadcast(trollPkt);
                 m_rlgl.Timer = 1.0f;
                 m_rlgl.IsRedLight = false; 
            }
        }
        else
        {
            // SWITCH TO GREEN
            m_rlgl.Timer = 3.0f + (m_system->m_dist(m_system->m_rng) % 4);
            
            PacketGameData pkt;
            pkt.Value = static_cast<int>(EGameDataType::GreenLight);
            m_server->Broadcast(pkt);
        }
    }

    // Check movement
    if (m_rlgl.IsRedLight && m_rlgl.GracePeriod <= 0.f)
    {
        for (auto& [peer, info] : m_server->GetPlayers())
        {
            if (info.playerState != EPlayerState::Playing)
                continue;

            float speedSq = info.velocityX * info.velocityX + info.velocityY * info.velocityY;
            if (speedSq > RedLightGreenLightState::MOVEMENT_THRESHOLD)
            {
                std::cout << "[RLGL] " << info.pseudo << " moved during Red Light! Speed: " << speedSq << std::endl;
                
                PacketGameData elimPkt;
                elimPkt.Value = static_cast<int>(EGameDataType::PlayerEliminated);
                elimPkt.ExtraData = info.pseudo;
                m_server->Broadcast(elimPkt);

                m_system->SetPlayerState(info, EPlayerState::Dead);

                auto& alive = m_system->m_alivePlayers;
                auto it = std::find(alive.begin(), alive.end(), info.pseudo);
                if (it != alive.end())
                    alive.erase(it);

                if (alive.empty())
                {
                    m_system->EndGame("Personne");
                    return;
                }
            }
        }
    }

    // Check finish line
    for (auto& [peer, info] : m_server->GetPlayers())
    {
        if (info.playerState != EPlayerState::Playing)
            continue;

        if (info.posX >= RedLightGreenLightState::FINISH_LINE_X)
        {
            // WINNER
            std::cout << "[RLGL] " << info.pseudo << " Crossed Finish Line!" << std::endl;

            PacketGameData winPkt;
            winPkt.Value = static_cast<int>(EGameDataType::PlayerFinished);
            winPkt.ExtraData = info.pseudo;
            m_server->Broadcast(winPkt);

            m_system->EndGame(info.pseudo);
            return;
        }
    }
}

void MiniGameRedLightGreenLightServer::OnPlayerDisconnect(PlayerInfo* player)
{
    auto& alive = m_system->m_alivePlayers;
    auto it = std::find(alive.begin(), alive.end(), player->pseudo);
    if (it != alive.end())
    {
        alive.erase(it);
        if (alive.empty())
        {
            m_system->EndGame("Personne");
        }
    }
}
