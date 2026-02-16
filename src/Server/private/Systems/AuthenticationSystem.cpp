#include "Systems/AuthenticationSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <iostream>
#include <chrono>
#include <vector>


void AuthenticationSystem::Init(GameServer* s)
{
    m_server = s;

    // --- CONNECT ---
    m_server->GetNetwork().OnPacket(OpCode::ConnectionState, 
    [this](GamePacket& rawPkt, ENetPeer* sender)
    {
        PacketConnectionState pkt;
        pkt.Deserialize(rawPkt);

        PlayerInfo* player = m_server->GetPlayerByPeer(sender);
        
        // LOGIN
        if (pkt.IsConnected) 
        {
            // Validation: pseudo length
            if (pkt.Pseudo.empty() || pkt.Pseudo.size() > 15)
            {
                PacketChat errorMsg;
                errorMsg.Sender = "SYSTEM";
                errorMsg.Message = "Pseudo invalide (1-15 caracteres).";
                errorMsg.ChannelName = "System";
                m_server->SendTo(sender, errorMsg);
                return;
            }

            // Validation: duplicate pseudo
            for (const auto& [peer, info] : m_server->GetPlayers())
            {
                if (info.pseudo == pkt.Pseudo && peer != sender)
                {
                    PacketChat errorMsg;
                    errorMsg.Sender = "SYSTEM";
                    errorMsg.Message = "Ce pseudo est deja utilise.";
                    errorMsg.ChannelName = "System";
                    m_server->SendTo(sender, errorMsg);
                    return;
                }
            }

            if (!player)
            {
                PlayerInfo newP;
                newP.peer = sender;
                newP.pseudo = pkt.Pseudo;
                newP.lastPacketTime = std::chrono::steady_clock::now();
                newP.colorID = static_cast<uint8_t>(m_colorDist(m_rng));
                
                auto& players = m_server->GetPlayers();
                if (players.empty())
                {
                    newP.isAdmin = true;
                    std::cout << "Premier joueur " << pkt.Pseudo << " devient ADMIN." << std::endl;
                }

                // Send existing player list to the newcomer
                for (const auto& [peer, info] : players)
                {
                    PacketPlayerList existingPkt;
                    existingPkt.Pseudo = info.pseudo;
                    existingPkt.ColorID = info.colorID;
                    existingPkt.State = info.playerState;
                    m_server->SendTo(sender, existingPkt);
                }

                auto [it, inserted] = players.emplace(sender, newP);
                player = &it->second;
                std::cout << "Nouveau joueur : " << pkt.Pseudo << " (Admin: " << newP.isAdmin << ")" << std::endl;
            }
            else
            {
                player->pseudo = pkt.Pseudo;
            }

            PacketConnectionState joinPkt;
            joinPkt.IsConnected = true;
            joinPkt.Pseudo = pkt.Pseudo;
            joinPkt.ColorID = player->colorID;
            m_server->Broadcast(joinPkt, sender);

            // Notify systems
            m_server->NotifyPlayerConnect(player);
        }
        else // LOGOUT
        {
             m_server->RemovePlayer(sender);
        }
    });

    // --- PING ---
    m_server->GetNetwork().OnPacket(OpCode::Ping, 
    [this](GamePacket& rawPkt, ENetPeer* sender) 
    {
        PlayerInfo* player = m_server->GetPlayerByPeer(sender);
        if (player) 
        {
            player->lastPacketTime = std::chrono::steady_clock::now();
        }

        PacketPing pong;
        m_server->SendTo(sender, pong);
    });
}

void AuthenticationSystem::Update(float dt)
{
    auto& players = m_server->GetPlayers();
    auto now = std::chrono::steady_clock::now();
    
    // Collect timed-out peers to avoid iterator invalidation
    std::vector<ENetPeer*> timedOut;
    for (const auto& [peer, info] : players)
    {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - info.lastPacketTime);
        if (duration.count() > TIMEOUT_SECONDS)
        {
            std::cout << "Timeout : " << info.pseudo << " Duration: " << duration.count() << "s" << std::endl;
            timedOut.push_back(peer);
        }
    }
    
    for (auto* peer : timedOut)
    {
        enet_peer_disconnect_now(peer, 0);
        m_server->RemovePlayer(peer);
    }
}
