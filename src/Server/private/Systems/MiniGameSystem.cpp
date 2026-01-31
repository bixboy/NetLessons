#include "../../public/Systems/MiniGameSystem.h"
#include "../../public/Core/GameServer.h"
#include <iostream>
#include <cstdlib>

MiniGameSystem::MiniGameSystem() : m_gameRunning(false), m_mysteryNumber(0)
{
}

void MiniGameSystem::Init(GameServer* server)
{
    // --- GAME START ---
    server->GetNetwork().OnPacket(OpCode::GameStart, [this, server](GamePacket& rawPkt, const sockaddr_in& sender) {
        if (!m_gameRunning)
        {
            m_gameRunning = true;
            m_mysteryNumber = rand() % 100;
            std::cout << "Jeu Lance ! Mystere = " << m_mysteryNumber << std::endl;
            
            PacketGameStart startPkt;
            server->Broadcast(startPkt);
        }
    });

    // --- GAME DATA ---
    server->GetNetwork().OnPacket(OpCode::GameData, [this, server](GamePacket& rawPkt, const sockaddr_in& sender) {
        if (!m_gameRunning) return;

        PacketGameData pkt;
        pkt.Deserialize(rawPkt);
        
        PlayerInfo* player = server->GetPlayerByAddr(sender);
        if (player) player->lastPacketTime = std::chrono::steady_clock::now();

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
    server->GetCommandManager().RegisterCommand("start", [this, server](PlayerInfo* p, const std::vector<std::string>& args){
          if (!m_gameRunning) {
              m_gameRunning = true;
              m_mysteryNumber = rand() % 100;
              PacketGameStart startPkt;
              server->Broadcast(startPkt);
          }
    });

    server->GetCommandManager().RegisterCommand("stop", [this, server](PlayerInfo* p, const std::vector<std::string>& args){
          m_gameRunning = false;
          PacketChat msg;
          msg.Sender = "SYSTEM";
          msg.Message = "Le serveur a arrete la partie.";
          server->Broadcast(msg);
    });
}
