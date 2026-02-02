#include "Systems/AuthenticationSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <iostream>
#include <chrono>
#include <cstdlib> // rand


void AuthenticationSystem::Init(GameServer* s)
{
    this->server = s;

    // --- CONNECT ---
    server->GetNetwork().OnPacket(OpCode::ConnectionState, 
    [s](GamePacket& rawPkt, const sockaddr_in& sender)
    {
        PacketConnectionState pkt;
        pkt.Deserialize(rawPkt);

        PlayerInfo* player = s->GetPlayerByAddr(sender);
        
        // LOGIN
        if (pkt.IsConnected) 
        {
            if (!player)
            {
                PlayerInfo newP;
                newP.address = sender;
                newP.pseudo = pkt.Pseudo;
                newP.lastPacketTime = std::chrono::steady_clock::now();
                newP.colorID = rand() % 8; // 8 couleurs disponibles
                
                auto& players = s->GetPlayers();
                if (players.empty())
                {
                    newP.isAdmin = true;
                    std::cout << "Premier joueur " << pkt.Pseudo << " devient ADMIN." << std::endl;
                }

                for (const auto& p : players)
                {
                    PacketPlayerList existingPkt;
                    existingPkt.Pseudo = p.pseudo;
                    existingPkt.ColorID = p.colorID;
                    s->SendTo(sender, existingPkt);
                }

                players.push_back(newP);
                player = &players.back();
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
            s->Broadcast(joinPkt, &sender);
        }
        else // LOGOUT
        {
             s->RemovePlayer(sender);
        }
    });

    // --- PING ---
    server->GetNetwork().OnPacket(OpCode::Ping, 
    [s](GamePacket& rawPkt, const sockaddr_in& sender) 
    {
        PlayerInfo* player = s->GetPlayerByAddr(sender);
        if (player) 
        {
            player->lastPacketTime = std::chrono::steady_clock::now();
        }

        // Always reply with Pong to keep client socket alive during login phase
        PacketPing pong;
        s->SendTo(sender, pong);
    });
}

void AuthenticationSystem::Update(float dt)
{
    auto& players = server->GetPlayers();
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = players.begin(); it != players.end();)
    {
        // ---- timeout ----
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->lastPacketTime);
        if (duration.count() > TIMEOUT_SECONDS)
        {
            std::cout << "Timeout : " << it->pseudo << " Duration: " << duration.count() << "s" << std::endl;
            
            PacketConnectionState leavePkt;
            leavePkt.IsConnected = false;
            leavePkt.Pseudo = it->pseudo;
            server->Broadcast(leavePkt, &it->address);
            
            it = players.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
