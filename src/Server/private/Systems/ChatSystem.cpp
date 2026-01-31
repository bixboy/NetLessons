#include "../../public/Systems/ChatSystem.h"
#include "../../public/Core/GameServer.h"
#include <iostream>
#include <algorithm>

void ChatSystem::Init(GameServer* server)
{
    // --- CHAT PACKET ---
    server->GetNetwork().OnPacket(OpCode::Chat, [server](GamePacket& rawPkt, const sockaddr_in& sender) {
        PacketChat pkt;
        pkt.Deserialize(rawPkt);

        PlayerInfo* player = server->GetPlayerByAddr(sender);
        if (player) 
        {
            player->lastPacketTime = std::chrono::steady_clock::now();
            
            // Verifier si c'est une commande
            if (server->GetCommandManager().ProcessCommand(player, pkt.Message))
            {
                return; // Commande traitee, on ne broadcast pas
            }

            std::cout << "[CHAT] " << player->pseudo << ": " << pkt.Message << std::endl;

            PacketChat broadcastChat;
            broadcastChat.Sender = player->pseudo;
            broadcastChat.Message = pkt.Message;
            server->Broadcast(broadcastChat);
        }
    });

    // --- COMMANDS ---
    server->GetCommandManager().RegisterCommand("help", [server](PlayerInfo* player, const std::vector<std::string>& args) {
        if (!player) return;
        
        PacketChat helpMsg;
        helpMsg.Sender = "SYSTEM";
        helpMsg.Message = "Commandes : /help, /kick <pseudo>, /stop, /start";
        server->SendTo(player->address, helpMsg);
    });

    server->GetCommandManager().RegisterCommand("kick", [server](PlayerInfo* requester, const std::vector<std::string>& args) {
         if (args.empty()) return;

         // Chercher le joueur
         std::string targetName = args[0];
         auto& players = server->GetPlayers();
         auto it = std::find_if(players.begin(), players.end(), [&](const PlayerInfo& p){
              return p.pseudo == targetName; 
         });

         if (it != players.end())
         {
             PacketChat kickMsg;
             kickMsg.Sender = "SYSTEM";
             kickMsg.Message = "Aurevoir " + targetName + " !";
             server->Broadcast(kickMsg);

             server->RemovePlayer(it->address);
         }
    });
}
