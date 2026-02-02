#include "Systems/MiniGameSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <chrono>
#include <iostream>


MiniGameSystem::MiniGameSystem() : m_gameRunning(false), m_mysteryNumber(0), m_rng(std::random_device{}()), m_dist(0, 99)
{
}

void MiniGameSystem::Init(GameServer* server)
{
    // --- GAME START ---
    server->GetNetwork().OnPacket(OpCode::GameStart, [this, server](GamePacket& rawPkt, const sockaddr_in& sender) 
    {
        if (!m_gameRunning)
        {
            m_gameRunning = true;
            m_mysteryNumber = m_dist(m_rng);
            std::cout << "Jeu Lance ! Mystere = " << m_mysteryNumber << std::endl;
            
            PacketGameStart startPkt;
            server->Broadcast(startPkt);
        }
    });
    
    // --- PLAYER STATE (Spectator) ---
    server->GetNetwork().OnPacket(OpCode::PlayerState, [this, server](GamePacket& rawPkt, const sockaddr_in& sender)
    {
        PacketPlayerState pkt;
        pkt.Deserialize(rawPkt);
        
        PlayerInfo* p = server->GetPlayerByAddr(sender);
        if (p)
        {
            p->isSpectator = pkt.IsSpectator;
            if (p->isSpectator)
            {
                 std::cout << "[GAME] " << p->pseudo << " est maintenant SPECTATEUR." << std::endl;
                 
                 if (m_gameRunning)
                 {
                     int activeCount = 0;
                     for(const auto& player : server->GetPlayers())
                     {
                         if (!player.isSpectator) 
                             activeCount++;
                     }
                     
                     if (activeCount == 0)
                     {
                         std::cout << "[GAME] Plus de joueurs actifs. Retour au Lobby." << std::endl;
                         m_gameRunning = false;
                         
                         PacketGameEnd endPkt;
                         server->Broadcast(endPkt);
                         
                         PacketChat msg;
                         msg.Sender = "SYSTEM";
                         msg.Message = "Faute de joueurs, retour au lobby.";
                         msg.ChannelName = "System";
                         server->Broadcast(msg);
                     }
                 }
            }
        }
    });

    // --- GAME DATA ---
    server->GetNetwork().OnPacket(OpCode::GameData, [this, server](GamePacket& rawPkt, const sockaddr_in& sender)
    {
        if (!m_gameRunning)
            return;

        PacketGameData pkt;
        pkt.Deserialize(rawPkt);
        
        PlayerInfo* player = server->GetPlayerByAddr(sender);
        if (player) 
            player->lastPacketTime = std::chrono::steady_clock::now();

        int guess = pkt.Value;
        PacketGameData response;
        
        if (guess < m_mysteryNumber) // PLUS
        {
            response.Value = 1;
            server->SendTo(sender, response);
        }
        else if (guess > m_mysteryNumber) // MOINS
        {
            response.Value = 2;
            server->SendTo(sender, response);
        }
        else // WIN
        {
            PacketGameResult winPkt;
            winPkt.WinnerName = (player ? player->pseudo : "Unknown");
            
            server->Broadcast(winPkt); 

            m_gameRunning = false;
        }
    });

    // --- COMMANDS ---
    server->GetCommandManager().RegisterCommand("start", [this, server](PlayerInfo* p, const std::vector<std::string>& args)
    {
          if (!p || !p->isAdmin) 
          {
             PacketChat msg; msg.Sender = "SYSTEM"; msg.Message = "Erreur: Vous n'etes pas ADMIN.";
             server->SendTo(p->address, msg);
             return;
          }

          if (!m_gameRunning) 
          {
              m_gameRunning = true;
              m_mysteryNumber = m_dist(m_rng);
              PacketGameStart startPkt;
              server->Broadcast(startPkt);
          }
    });

    server->GetCommandManager().RegisterCommand("stop", [this, server](PlayerInfo* p, const std::vector<std::string>& args)
    {
          if (!p || !p->isAdmin) 
          {
             PacketChat msg; msg.Sender = "SYSTEM"; msg.Message = "Erreur: Vous n'etes pas ADMIN.";
             server->SendTo(p->address, msg);
             return;
          }
          
          m_gameRunning = false;
          PacketChat msg;
          msg.Sender = "SYSTEM";
          msg.Message = "Le serveur a arrete la partie.";
          msg.ChannelName = "System";
          server->Broadcast(msg);
    });
}
