#include "Systems/ChatSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <iostream>
#include <algorithm>
#include <chrono>


void ChatSystem::Init(GameServer* server)
{
    // --- CHAT PACKET ---
    server->GetNetwork().OnPacket(OpCode::Chat, [server](GamePacket& rawPkt, ENetPeer* sender) 
    {
        PacketChat pkt;
        pkt.Deserialize(rawPkt);

        PlayerInfo* player = server->GetPlayerByPeer(sender);
        if (!player) 
            return;

        player->lastPacketTime = std::chrono::steady_clock::now();
        
        // Validate message length
        if (pkt.Message.empty() || pkt.Message.size() > 500)
            return;
            
        if (server->GetCommandManager().ProcessCommand(player, pkt.Message))
            return;

        // Private Message
        if (!pkt.Target.empty())
        {
            PlayerInfo* target = nullptr;
            for (auto& [peer, info] : server->GetPlayers())
            {
                if (info.pseudo == pkt.Target)
                {
                    target = &info;
                    break;
                }
            }

            if (target)
            {
                PacketChat pm;
                pm.Sender = player->pseudo;
                pm.Message = pkt.Message;
                pm.Target = target->pseudo;
                pm.ChannelName = pkt.ChannelName;
                server->SendTo(target->peer, pm);
                server->SendTo(player->peer, pm);

                std::cout << "[WHISPER] " << player->pseudo << " -> " << target->pseudo << ": " << pkt.Message << std::endl;
            }
            else
            {
                PacketChat errorMsg;
                errorMsg.Sender = "SYSTEM";
                errorMsg.Message = "Joueur introuvable : " + pkt.Target;
                errorMsg.ChannelName = "System";
                server->SendTo(player->peer, errorMsg);
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
    });

    // --- COMMANDS ---
    server->GetCommandManager().RegisterCommand("help", [server](PlayerInfo* player, const std::vector<std::string>& args) 
    {
        if (!player) 
            return;
        
        PacketChat helpMsg;
        helpMsg.Sender = "SYSTEM";
        helpMsg.Message = "Commandes : /help, /kick <pseudo>, /stop, /start";
        server->SendTo(player->peer, helpMsg);
    });

    server->GetCommandManager().RegisterCommand("kick", [server](PlayerInfo* requester, const std::vector<std::string>& args) 
    {
         // Fix: check nullptr BEFORE dereferencing
         if (!requester)
             return;
             
         if (!requester->isAdmin) 
         {
             PacketChat msg; 
             msg.Sender = "SYSTEM"; 
             msg.Message = "Erreur: Vous n'etes pas ADMIN.";
             msg.ChannelName = "System";
             server->SendTo(requester->peer, msg);
             return;
         }

         if (args.empty()) 
            return;

         std::string targetName = args[0];
         
         PlayerInfo* target = nullptr;
         ENetPeer* targetPeer = nullptr;
         for (auto& [peer, info] : server->GetPlayers())
         {
             if (info.pseudo == targetName)
             {
                 target = &info;
                 targetPeer = peer;
                 break;
             }
         }

         if (target)
         {
             PacketChat kickMsg;
             kickMsg.Sender = "SYSTEM";
             kickMsg.Message = "Aurevoir " + targetName + " !";
             kickMsg.ChannelName = "System";
             server->Broadcast(kickMsg);

             server->RemovePlayer(targetPeer);
         }
    });
}
