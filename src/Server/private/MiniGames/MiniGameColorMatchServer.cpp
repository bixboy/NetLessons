#include "MiniGames/MiniGameColorMatchServer.h"
#include "Core/GameServer.h"
#include "Systems/MiniGameSystem.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

void MiniGameColorMatchServer::Start(GameServer* server, MiniGameSystem* system)
{
    m_server = server;
    m_system = system;
    
    PacketGameStart startPkt;
    startPkt.GameID = 3;
    server->Broadcast(startPkt);

    m_system->m_alivePlayers.clear();
    for (auto& [peer, info] : server->GetPlayers())
    {
        m_system->SetPlayerState(info, EPlayerState::Playing);
        m_system->m_alivePlayers.push_back(info.pseudo);
        
        std::cout << " -> Added " << info.pseudo << " to ColorMatch. Pos: " << info.posX << "," << info.posY << std::endl;
        
        info.posX = static_cast<float>(rand()) / RAND_MAX;
        info.posY = static_cast<float>(rand()) / RAND_MAX;

        PacketPlayerPosition posPkt;
        posPkt.Pseudo = info.pseudo;
        posPkt.X = info.posX;
        posPkt.Y = info.posY;
        server->Broadcast(posPkt);
    }
    
    std::cout << "[COLOR MATCH] Init with " << m_system->m_alivePlayers.size() << " players." << std::endl;

    // Init Logic
    m_colorMatch.RoundDuration = 5.0f;
    m_colorMatch.Timer = 3.0f;
    m_colorMatch.Phase = ColorMatchState::EPhase::Warmup;

    // Generate Grid (4x4)
    m_colorMatch.GridColors.resize(16);
    std::string gridStr = "";
    for (int i = 0; i < 16; ++i)
    {
        m_colorMatch.GridColors[i] = m_system->m_dist(m_system->m_rng) % 4;
        gridStr += std::to_string(m_colorMatch.GridColors[i]) + ",";
    }

    // Broadcast Grid
    PacketGameData gridPkt;
    gridPkt.Value = static_cast<int>(EGameDataType::ColorGrid);
    gridPkt.ExtraData = gridStr;
    server->Broadcast(gridPkt);

    std::cout << "[COLOR MATCH] Started! Grid generated." << std::endl;
}

void MiniGameColorMatchServer::Update(float dt)
{
    m_colorMatch.Timer -= dt;

    if (m_colorMatch.Timer <= 0.f)
    {
        using Phase = ColorMatchState::EPhase;
        
        switch (m_colorMatch.Phase)
        {
        case Phase::Warmup:
        case Phase::Reset:
            {
                m_colorMatch.Phase = Phase::Tension;
                m_colorMatch.Timer = 5.0f;
                m_colorMatch.TargetColor = m_system->m_dist(m_system->m_rng) % 4;
                
                PacketGameData pkt;
                pkt.Value = static_cast<int>(EGameDataType::ColorTension);
                m_server->Broadcast(pkt);
            }
            break;

        case Phase::Tension:
            {
                m_colorMatch.Phase = Phase::Reveal;
                m_colorMatch.Timer = m_colorMatch.RoundDuration;
                
                PacketGameData pkt;
                pkt.Value = static_cast<int>(EGameDataType::ColorReveal);
                pkt.Timer = m_colorMatch.RoundDuration;
                pkt.ExtraData = std::to_string(m_colorMatch.TargetColor);
                m_server->Broadcast(pkt);

                std::cout << "[COLOR MATCH] REVEAL! Target: " << m_colorMatch.TargetColor << std::endl;
            }
            break;

        case Phase::Reveal:
            {
                m_colorMatch.Phase = Phase::Elimination;
                m_colorMatch.Timer = 2.0f;

                PacketGameData pkt;
                pkt.Value = static_cast<int>(EGameDataType::ColorElimination);
                m_server->Broadcast(pkt);
                
                // Check Players
                for (auto& [peer, info] : m_server->GetPlayers())
                {
                    if (info.playerState != EPlayerState::Playing) continue;

                    int gx = static_cast<int>(info.posX * 4.f);
                    int gy = static_cast<int>(info.posY * 4.f);
                    gx = std::clamp(gx, 0, 3);
                    gy = std::clamp(gy, 0, 3);
                    
                    int tileIndex = gy * 4 + gx;
                    
                    bool isSafe = (std::cmp_equal(m_colorMatch.GridColors[tileIndex], m_colorMatch.TargetColor));

                    if (!isSafe)
                    {
                        float strictX = info.posX * 4.f;
                        float strictY = info.posY * 4.f;
                        float margin = 0.15f; 

                        float fracX = strictX - gx; 
                        float fracY = strictY - gy; 

                        auto CheckTile = [&](int tx, int ty) -> bool 
                        {
                            if (tx < 0 || tx > 3 || ty < 0 || ty > 3) 
                                return false;
                            
                            int idx = ty * 4 + tx;
                            return std::cmp_not_equal(m_colorMatch.GridColors[idx], m_colorMatch.TargetColor);
                        };

                        if (fracX < margin && CheckTile(gx - 1, gy))
                        {
                            isSafe = true;
                        }
                        else if (fracX > (1.f - margin) && CheckTile(gx + 1, gy))
                        {
                            isSafe = true;
                        }
                        
                        if (!isSafe) 
                        {
                            if (fracY < margin && CheckTile(gx, gy - 1))
                            {
                                isSafe = true;
                            }
                            else if (fracY > (1.f - margin) && CheckTile(gx, gy + 1))
                            {
                                isSafe = true;
                            }
                        }
                        
                        if (!isSafe) 
                        {
                             if (fracX < margin && fracY < margin && CheckTile(gx - 1, gy - 1))
                             {
                                 isSafe = true;
                             }
                             else if (fracX > (1.f - margin) && fracY < margin && CheckTile(gx + 1, gy - 1))
                             {
                                 isSafe = true;
                             }
                             else if (fracX < margin && fracY > (1.f - margin) && CheckTile(gx - 1, gy + 1))
                             {
                                 isSafe = true;
                             }
                             else if (fracX > (1.f - margin) && fracY > (1.f - margin) && CheckTile(gx + 1, gy + 1))
                             {
                                 isSafe = true;
                             }
                        }
                    }

                    if (!isSafe)
                    {
                         // DIE
                         int tileColor = m_colorMatch.GridColors[tileIndex];
                         std::cout << " -> " << info.pseudo << " died on " << tileColor << " (Target: " << m_colorMatch.TargetColor << ")" << std::endl;
                         m_system->SetPlayerState(info, EPlayerState::Dead);
                         
                         auto it = std::find(m_system->m_alivePlayers.begin(), m_system->m_alivePlayers.end(), info.pseudo);
                         if (it != m_system->m_alivePlayers.end()) 
                         {
                             m_system->m_alivePlayers.erase(it);
                         }
                    }
                }

                // Check Win
                if (m_system->m_alivePlayers.empty())
                {
                     m_system->EndGame("Personne");
                     return;
                }
                if (m_system->m_alivePlayers.size() == 1)
                {
                     m_system->EndGame(m_system->m_alivePlayers[0]);
                }
            }
            
            break;

        case Phase::Elimination:
            {
                m_colorMatch.Phase = Phase::Reset;
                m_colorMatch.Timer = 1.0f;
                m_colorMatch.RoundDuration = (std::max)(2.0f, m_colorMatch.RoundDuration - 0.5f);
                
                for (int i = 0; i < 16; ++i)
                {
                    m_colorMatch.GridColors[i] = m_system->m_dist(m_system->m_rng) % 4;
                }
                
                std::string gridStr = "";
                for(int c : m_colorMatch.GridColors) gridStr += std::to_string(c) + ",";
                
                PacketGameData gridPkt;
                gridPkt.Value = static_cast<int>(EGameDataType::ColorGrid);
                gridPkt.ExtraData = gridStr;
                m_server->Broadcast(gridPkt);
            }
            
            break;
        }
    }
}

void MiniGameColorMatchServer::OnPlayerDisconnect(PlayerInfo* player)
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
        else if (alive.size() == 1)
        {
             m_system->EndGame(alive[0]);
        }
    }
}

void MiniGameColorMatchServer::OnPlayerConnect(PlayerInfo* player)
{
    std::string gridStr = "";
    for (int i = 0; i < 16; ++i)
    {
        gridStr += std::to_string(m_colorMatch.GridColors[i]) + ",";
    }

    PacketGameData gridPkt;
    gridPkt.Value = static_cast<int>(EGameDataType::ColorGrid);
    gridPkt.ExtraData = gridStr;
    m_server->SendTo(player->peer, gridPkt);

    if (m_colorMatch.Phase == ColorMatchState::EPhase::Tension)
    {
        PacketGameData pkt;
        pkt.Value = static_cast<int>(EGameDataType::ColorTension);
        m_server->SendTo(player->peer, pkt);
    }
    else if (m_colorMatch.Phase == ColorMatchState::EPhase::Reveal)
    {
        PacketGameData pkt;
        pkt.Value = static_cast<int>(EGameDataType::ColorReveal);
        pkt.Timer = m_colorMatch.Timer;
        pkt.ExtraData = std::to_string(m_colorMatch.TargetColor);
        m_server->SendTo(player->peer, pkt);
    }
}



