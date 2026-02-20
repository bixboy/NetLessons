#include "Systems/MiniGameSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <chrono>
#include <iostream>

#include "MiniGames/MiniGameJustePrixServer.h"
#include "MiniGames/MiniGameHotPotatoServer.h"
#include "MiniGames/MiniGameRedLightGreenLightServer.h"
#include "MiniGames/MiniGameColorMatchServer.h"


MiniGameSystem* MiniGameSystem::s_instance = nullptr;

MiniGameSystem::MiniGameSystem() : m_rng(std::random_device{}()), m_dist(0, 99)
{
    s_instance = this;
}

MiniGameSystem::~MiniGameSystem()
{
    if (s_instance == this)
        s_instance = nullptr;
}

bool MiniGameSystem::IsIceMode() const
{
    if (m_currentGame)
        return m_currentGame->IsIceMode();
    
    return false;
}


void MiniGameSystem::SetPlayerState(PlayerInfo& player, EPlayerState newState)
{
    player.playerState = newState;

    PacketPlayerState pkt;
    pkt.Pseudo = player.pseudo;
    pkt.State  = newState;
    m_server->Broadcast(pkt);
}


// ========
// EndGame 
// ========
void MiniGameSystem::EndGame(const std::string& winnerName)
{
    PacketGameResult winPkt;
    winPkt.WinnerName = winnerName;
    m_server->Broadcast(winPkt);

    m_gameRunning = false;
    m_currentGame.reset();

    for (auto& [peer, info] : m_server->GetPlayers())
    {
        SetPlayerState(info, EPlayerState::Lobby);
    }

    PacketGameData statePkt;
    statePkt.Value = static_cast<int>(EGameDataType::GameStateChanged);
    statePkt.ExtraData = "0";
    statePkt.Timer = 0.f;
    m_server->Broadcast(statePkt);

    std::cout << "[GAME] Partie terminee. Gagnant: " << winnerName << std::endl;
}

void MiniGameSystem::Init(GameServer* server)
{
    m_server = server;

    server->OnPushHit = [this](const std::string& pusher, const std::string& target)
    {
        HandlePushHit(pusher, target);
    };

    // --- GAME START ---
    server->GetNetwork().OnPacket(OpCode::GameStart, [this, server](GamePacket& rawPkt, ENetPeer* sender) 
    {
        PacketGameStart pkt;
        pkt.Deserialize(rawPkt);

        if (!m_gameRunning)
        {
            m_activeGameID = pkt.GameID;
            
            switch (m_activeGameID)
            {
            case 0: 
                m_currentGame = std::make_unique<MiniGameJustePrixServer>();
                break;
                
            case 1:
                m_currentGame = std::make_unique<MiniGameHotPotatoServer>();
                break;
                
            case 2:
                m_currentGame = std::make_unique<MiniGameRedLightGreenLightServer>();
                break;
                
            case 3: 
                m_currentGame = std::make_unique<MiniGameColorMatchServer>();
                break;
                
            default: return;
            }

            if (m_currentGame)
            {
                m_gameRunning = true;
                m_currentGame->Start(server, this);

                PacketGameData statePkt;
                statePkt.Value = static_cast<int>(EGameDataType::GameStateChanged);
                statePkt.ExtraData = "1";
                statePkt.Timer = static_cast<float>(m_activeGameID);
                server->Broadcast(statePkt);
            }
        }
        else
        {
            std::cout << "Late join request from peer. Sending to Spectator mode." << std::endl;
            
            PlayerInfo* player = server->GetPlayerByPeer(sender);
            if (!player)
                return;

            PacketGameStart startPkt;
            startPkt.GameID = m_activeGameID;
            server->SendTo(sender, startPkt);

            SetPlayerState(*player, EPlayerState::Spectating);
        }
    });
    
    // --- PLAYER STATE ---
    server->GetNetwork().OnPacket(OpCode::PlayerState, [this, server](GamePacket& rawPkt, ENetPeer* sender)
    {
        PacketPlayerState pkt;
        pkt.Deserialize(rawPkt);
        
        PlayerInfo* p = server->GetPlayerByPeer(sender);
        if (!p)
            return;

        EPlayerState requested = pkt.State;

        if (requested == EPlayerState::Lobby && p->playerState == EPlayerState::Playing)
        {
             if (m_gameRunning && m_currentGame)
                m_currentGame->OnPlayerDisconnect(p);

             SetPlayerState(*p, EPlayerState::Lobby);
        }
    });
    
    // --- GAME DATA ---
    server->GetNetwork().OnPacket(OpCode::GameData, [this, server](GamePacket& rawPkt, ENetPeer* sender)
    {
        if (!m_gameRunning || !m_currentGame)
            return;

        m_currentGame->HandlePacket(OpCode::GameData, rawPkt, sender);
    });

    // --- COMMANDS ---
    server->GetCommandManager().RegisterCommand("start", [this, server](PlayerInfo* p, const std::vector<std::string>& args)
    {
          if (!p)
              return;
              
          if (!p->isAdmin) 
          {
             PacketChat msg;
             msg.Sender = "SYSTEM";
             msg.Message = "Erreur: Vous n'etes pas ADMIN.";
             server->SendTo(p->peer, msg);
             return;
          }

          if (!m_gameRunning) 
          {
              int gameID = 0;
              if (!args.empty()) 
              {
                  try
                  {
                      gameID = std::stoi(args[0]);
                  } catch(...) {}
              }

              PacketGameStart startPkt;
              startPkt.GameID = gameID;
              
              m_activeGameID = gameID;
              switch (m_activeGameID)
              {
                case 0:
                  m_currentGame = std::make_unique<MiniGameJustePrixServer>();
                  break;
                  
                case 1:
                  m_currentGame = std::make_unique<MiniGameHotPotatoServer>();
                  break;
                  
                case 2:
                  m_currentGame = std::make_unique<MiniGameRedLightGreenLightServer>();
                  break;
                  
                case 3:
                  m_currentGame = std::make_unique<MiniGameColorMatchServer>();
                  break;
                  
                default:
                  m_currentGame = std::make_unique<MiniGameJustePrixServer>(); m_activeGameID=0;
                  break;
              }

              if (m_currentGame)
              {
                  m_gameRunning = true;
                  m_currentGame->Start(server, this); 

                  PacketGameData statePkt;
                  statePkt.Value = static_cast<int>(EGameDataType::GameStateChanged);
                  statePkt.ExtraData = "1";
                  statePkt.Timer = static_cast<float>(m_activeGameID);
                  server->Broadcast(statePkt);
              }
          }
    });

    server->GetCommandManager().RegisterCommand("stop", [this, server](PlayerInfo* p, const std::vector<std::string>& args)
    {
          if (!p)
              return;
              
          if (!p->isAdmin) 
          {
             PacketChat msg;
             msg.Sender = "SYSTEM";
             msg.Message = "Erreur: Vous n'etes pas ADMIN.";
             server->SendTo(p->peer, msg);
             return;
          }
          
          if (m_gameRunning)
              EndGame("Personne");

          PacketChat msg;
          msg.Sender = "SYSTEM";
          msg.Message = "Le serveur a arrete la partie.";
          msg.ChannelName = "System";
          server->Broadcast(msg);
    });
}

// =======
// UPDATE
// =======
void MiniGameSystem::Update(float dt)
{
    if (!m_gameRunning || !m_currentGame)
        return;

    m_currentGame->Update(dt);
}

// ====================
// OnPlayerDisconnect 
// ====================
void MiniGameSystem::OnPlayerDisconnect(PlayerInfo* player)
{
    if (!m_gameRunning || !m_currentGame || !player)
        return;

    m_currentGame->OnPlayerDisconnect(player);
}

// ===============
// HandlePushHit 
// ===============
void MiniGameSystem::HandlePushHit(const std::string& pusher, const std::string& target)
{
    if (!m_gameRunning || !m_currentGame)
        return;

    m_currentGame->OnPush(pusher, target);
}

// ================
// OnPlayerConnect
// ================
void MiniGameSystem::OnPlayerConnect(PlayerInfo* player)
{
    PacketGameData statePkt;
    statePkt.Value = static_cast<int>(EGameDataType::GameStateChanged);
    statePkt.ExtraData = m_gameRunning ? "1" : "0";
    statePkt.Timer = static_cast<float>(m_activeGameID);
    
    m_server->SendTo(player->peer, statePkt);

    PacketPlayerState newPkt;
    newPkt.Pseudo = player->pseudo;
    newPkt.State = player->playerState;
    m_server->Broadcast(newPkt);

    for (const auto& [peer, info] : m_server->GetPlayers())
    {
        if (info.pseudo == player->pseudo)
            continue;
        
        PacketPlayerState existPkt;
        existPkt.Pseudo = info.pseudo;
        existPkt.State  = info.playerState;
        m_server->SendTo(player->peer, existPkt);
    }
    
    if (m_gameRunning && m_currentGame)
    {
        m_currentGame->OnPlayerConnect(player);
    }
}
