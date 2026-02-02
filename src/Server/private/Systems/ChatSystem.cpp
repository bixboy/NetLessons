#include "Systems/ChatSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <iostream>
#include <algorithm>
#include <chrono>


void ChatSystem::Init(GameServer* server)
{
    // --- CHAT PACKET ---
    server->GetNetwork().OnPacket(OpCode::Chat, [server](GamePacket& rawPkt, const sockaddr_in& sender) 
    {
        PacketChat pkt;
        pkt.Deserialize(rawPkt);

        PlayerInfo* player = server->GetPlayerByAddr(sender);
        if (player) 
        {
            player->lastPacketTime = std::chrono::steady_clock::now();
            
            if (server->GetCommandManager().ProcessCommand(player, pkt.Message))
            {
                return;
            }

            // Private Message
            if (!pkt.Target.empty())
            {
                auto& players = server->GetPlayers();
                auto it = std::find_if(players.begin(), players.end(), [&](const PlayerInfo& p) {
                    return p.pseudo == pkt.Target; 
                });

                if (it != players.end())
                {
                    // To Recipient
                    PacketChat pm;
                    pm.Sender = player->pseudo;
                    pm.Message = pkt.Message;
                    pm.Target = it->pseudo;
                    pm.ChannelName = pkt.ChannelName;
                    server->SendTo(it->address, pm);

                    // To Sender
                    server->SendTo(player->address, pm);

                    std::cout << "[WHISPER] " << player->pseudo << " -> " << it->pseudo << ": " << pkt.Message << std::endl;
                }
                else
                {
                    PacketChat errorMsg;
                    errorMsg.Sender = "SYSTEM";
                    errorMsg.Message = "Joueur introuvable : " + pkt.Target;
                    errorMsg.ChannelName = "System";
                    server->SendTo(player->address, errorMsg);
                }
            }
            else
            {
                // GLOBAL BROADCAST
                std::cout << "[CHAT] " << player->pseudo << ": " << pkt.Message << std::endl;

                PacketChat broadcastChat;
                broadcastChat.Sender = player->pseudo;
                broadcastChat.Message = pkt.Message;
                broadcastChat.ChannelName = pkt.ChannelName;
                server->Broadcast(broadcastChat);
            }
        }
    });

    // --- COMMANDS ---
    server->GetCommandManager().RegisterCommand("help", [server](PlayerInfo* player, const std::vector<std::string>& args) 
    {
        if (!player) 
            return;
        
        PacketChat helpMsg;
        helpMsg.Sender = "SYSTEM";
        helpMsg.Message = "Commandes : /help, /kick <pseudo>, /stop, /start";
        server->SendTo(player->address, helpMsg);
    });

    server->GetCommandManager().RegisterCommand("kick", [server](PlayerInfo* requester, const std::vector<std::string>& args) 
    {
         if (!requester || !requester->isAdmin) 
         {
             PacketChat msg; 
             msg.Sender = "SYSTEM"; 
             msg.Message = "Erreur: Vous n'etes pas ADMIN.";
             msg.ChannelName = "System";
             server->SendTo(requester->address, msg);
             return;
         }

         if (args.empty()) 
            return;

         std::string targetName = args[0];
         auto& players = server->GetPlayers();
        
         auto it = std::find_if(players.begin(), players.end(), 
            [&](const PlayerInfo& p)
            {
                return p.pseudo == targetName; 
            });

         if (it != players.end())
         {
             PacketChat kickMsg;
             kickMsg.Sender = "SYSTEM";
             kickMsg.Message = "Aurevoir " + targetName + " !";
             kickMsg.ChannelName = "System";
             server->Broadcast(kickMsg);

             server->RemovePlayer(it->address);
         }
    });
}
