#include "../public/GameServer.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>

GameServer::GameServer() : m_gameRunning(false), m_mysteryNumber(0)
{
}

GameServer::~GameServer()
{
}

bool GameServer::Initialize()
{
    if (!m_network.Start(PORT))
    {
        return false;
    }

    auto handler = [this](GamePacket& pkt, const sockaddr_in& sender) {
        HandlePacket(pkt, sender);
    };

    m_network.OnPacket(PacketType::Connect, handler);
    m_network.OnPacket(PacketType::Disconnect, handler);
    m_network.OnPacket(PacketType::GameStart, handler);
    m_network.OnPacket(PacketType::GameData, handler);
    m_network.OnPacket(PacketType::Chat, handler);
    m_network.OnPacket(PacketType::Ping, handler);
    
    return true;
}

void GameServer::Run()
{
    std::cout << "Server loop running..." << std::endl;
    while (true)
    {
        m_network.PollEvents();
        
        auto now = std::chrono::steady_clock::now();
        for (auto it = m_players.begin(); it != m_players.end(); )
        {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->lastPacketTime);
            if (duration.count() > TIMEOUT_SECONDS)
            {
                std::cout << "Timeout : " << it->pseudo << std::endl;
                
                GamePacket leavePkt;
                leavePkt << (int)PacketType::Disconnect << it->pseudo;
                Broadcast(leavePkt, &it->address);
                
                it = m_players.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::HandlePacket(GamePacket& pkt, const sockaddr_in& sender)
{
    int typeInt = 0;
    pkt >> typeInt; 
    PacketType type = static_cast<PacketType>(typeInt);

    PlayerInfo* player = GetPlayerByAddr(sender);

    if (player)
    {
        player->lastPacketTime = std::chrono::steady_clock::now();
    }

    // --- CONNEXION ---
    if (type == PacketType::Connect)
    {
        std::string pseudo;
        pkt >> pseudo;

        if (!player)
        {
            PlayerInfo newP;
            newP.address = sender;
            newP.pseudo = pseudo;
            newP.lastPacketTime = std::chrono::steady_clock::now();
            
            for (const auto& p : m_players)
            {
                GamePacket listPkt;
                listPkt << (int)PacketType::Connect << p.pseudo;
                
                GamePacket existingPkt;
                existingPkt << (int)PacketType::PlayerList << p.pseudo;
                SendTo(sender, existingPkt);
            }

            m_players.push_back(newP);
            player = &m_players.back();
            std::cout << "Nouveau joueur : " << pseudo << std::endl;
        }
        else
        {
            player->pseudo = pseudo;
        }

        GamePacket joinPkt;
        joinPkt << (int)PacketType::Connect << pseudo;
        Broadcast(joinPkt, &sender);
        return;
    }

    if (!player) 
        return;

    // --- START GAME ---
    if (type == PacketType::GameStart && !m_gameRunning)
    {
        m_gameRunning = true;
        m_mysteryNumber = rand() % 100;
        std::cout << "Jeu Lance ! Mystere = " << m_mysteryNumber << std::endl;
        
        GamePacket startPkt;
        startPkt << (int)PacketType::GameStart;
        Broadcast(startPkt);
    }
    // --- CHAT ---
    else if (type == PacketType::Chat)
    {
        std::string msg;
        pkt >> msg;
        
        std::cout << "[CHAT] " << player->pseudo << ": " << msg << std::endl;
        
        GamePacket chatPkt;
        chatPkt << (int)PacketType::Chat << (player->pseudo) << msg; 
        
        Broadcast(chatPkt);
    }
    // --- JEU ---
    else if (type == PacketType::GameData && m_gameRunning)
    {
        int guess = 0;
        pkt >> guess;

        GamePacket response;
        
        if (guess < m_mysteryNumber) // C'est PLUS
        {
            response << (int)PacketType::GameData << 1;
            SendTo(sender, response);
        }
        else if (guess > m_mysteryNumber) // C'est MOINS
        {
            response << (int)PacketType::GameData << 2;
            SendTo(sender, response);
        }
        else // VICTOIRE
        {
            GamePacket winPkt;
            winPkt << static_cast<int>(PacketType::Win);
            SendTo(sender, winPkt);

            GamePacket losePkt;
            losePkt << static_cast<int>(PacketType::Lose) << player->pseudo;
            Broadcast(losePkt, &sender);

            m_gameRunning = false;
        }
    }
    else if (type == PacketType::Disconnect)
    {
        RemovePlayer(sender);
    }
}

PlayerInfo* GameServer::GetPlayerByAddr(const sockaddr_in& addr)
{
    for (auto& p : m_players)
    {
        if (p.address.sin_addr.s_addr == addr.sin_addr.s_addr && p.address.sin_port == addr.sin_port)
        {
            return &p;
        }
    }
    
    return nullptr;
}

void GameServer::RemovePlayer(const sockaddr_in& addr)
{
    auto it = std::find_if(m_players.begin(), m_players.end(), [&](const PlayerInfo& p) 
        {
            return p.address.sin_addr.s_addr == addr.sin_addr.s_addr && p.address.sin_port == addr.sin_port;
        });

    if (it != m_players.end())
    {
        std::cout << "Deconnexion : " << it->pseudo << std::endl;
        
        GamePacket leavePkt;
        leavePkt << (int)PacketType::Disconnect << it->pseudo;
        Broadcast(leavePkt, &addr);
        
        m_players.erase(it);
    }
}

void GameServer::Broadcast(GamePacket& pkt, const sockaddr_in* senderToIgnore)
{
    for (auto& p : m_players)
    {
        if (senderToIgnore != nullptr && p.address.sin_addr.s_addr == senderToIgnore->sin_addr.s_addr && p.address.sin_port == senderToIgnore->sin_port)
            continue;
        
        SendTo(p.address, pkt);
    }
}

void GameServer::SendTo(const sockaddr_in& target, GamePacket& pkt)
{
    m_network.SendTo(pkt, target);
}