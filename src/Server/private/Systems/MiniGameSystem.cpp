#include "Systems/MiniGameSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <chrono>
#include <iostream>
#include <algorithm>


MiniGameSystem::MiniGameSystem()
    : m_rng(std::random_device{}()), m_dist(0, 99)
{
}


// ============================================================
// SetPlayerState — single authority point for state transitions
// ============================================================
void MiniGameSystem::SetPlayerState(PlayerInfo& player, EPlayerState newState)
{
    player.playerState = newState;

    // Broadcast to all clients
    PacketPlayerState pkt;
    pkt.Pseudo = player.pseudo;
    pkt.State  = newState;
    m_server->Broadcast(pkt);
}


// ============================================================
// EndGame — single authority point for game termination
// ============================================================
void MiniGameSystem::EndGame(const std::string& winnerName)
{
    // 1. Broadcast result
    PacketGameResult winPkt;
    winPkt.WinnerName = winnerName;
    m_server->Broadcast(winPkt);

    // 2. Mark game as stopped
    m_gameRunning = false;

    // 3. Reset ALL players to Lobby
    for (auto& [peer, info] : m_server->GetPlayers())
        SetPlayerState(info, EPlayerState::Lobby);

    // 4. Broadcast game state change
    PacketGameData statePkt;
    statePkt.Value     = static_cast<int>(EGameDataType::GameStateChanged);
    statePkt.ExtraData = "0";
    statePkt.Timer     = 0.f;
    m_server->Broadcast(statePkt);

    std::cout << "[GAME] Partie terminee. Gagnant: " << winnerName << std::endl;
}


// ============================================================
// Init — register packet handlers & commands
// ============================================================
void MiniGameSystem::Init(GameServer* server)
{
    m_server = server;

    // Register push callback for bomb transfer
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
            if (pkt.GameID == 0)
            {
                // Juste Prix
                m_activeGameID = 0;
                m_gameRunning = true;
                m_mysteryNumber = m_dist(m_rng);
                std::cout << "Jeu Lance ! ID=0 Mystere = " << m_mysteryNumber << std::endl;

                // Set only Lobby players to Playing
                for (auto& [peer, info] : server->GetPlayers())
                {
                    if (info.playerState == EPlayerState::Lobby)
                        SetPlayerState(info, EPlayerState::Playing);
                }

                PacketGameStart startPkt;
                startPkt.GameID = 0;
                server->Broadcast(startPkt);

                // Notify Game Running
                PacketGameData statePkt;
                statePkt.Value     = static_cast<int>(EGameDataType::GameStateChanged);
                statePkt.ExtraData = "1";
                statePkt.Timer     = static_cast<float>(m_activeGameID);
                server->Broadcast(statePkt);
            }
            else if (pkt.GameID == 1)
            {
                // Hot Potato
                StartHotPotato(server);

                // Notify Game Running
                PacketGameData statePkt;
                statePkt.Value     = static_cast<int>(EGameDataType::GameStateChanged);
                statePkt.ExtraData = "1";
                statePkt.Timer     = static_cast<float>(m_activeGameID);
                server->Broadcast(statePkt);
            }
        }
        else
        {
            // Game ALREADY running -> Late Join as Spectator
            std::cout << "Late join request from peer. Sending to Spectator mode." << std::endl;
            
            PlayerInfo* player = server->GetPlayerByPeer(sender);
            if (!player) return;

            // 1. Send Game Start (so they switch screen)
            PacketGameStart startPkt;
            startPkt.GameID = m_activeGameID;
            server->SendTo(sender, startPkt);

            // 2. Set as Spectator (server-authoritative)
            SetPlayerState(*player, EPlayerState::Spectating);
        }
    });
    
    // --- PLAYER STATE (Client requests a state change) ---
    server->GetNetwork().OnPacket(OpCode::PlayerState, [this, server](GamePacket& rawPkt, ENetPeer* sender)
    {
        PacketPlayerState pkt;
        pkt.Deserialize(rawPkt);
        
        PlayerInfo* p = server->GetPlayerByPeer(sender);
        if (!p) return;

        // Server-authoritative: validate the transition
        EPlayerState requested = pkt.State;

        // Only allow: Playing->Lobby (via /lobby command)
        // Spectating->Lobby transitions are handled by EndGame()
        if (requested == EPlayerState::Lobby && p->playerState == EPlayerState::Playing)
        {
            SetPlayerState(*p, EPlayerState::Lobby);

            // Check if game should end
            if (m_gameRunning)
            {
                if (m_activeGameID == 1) // Hot Potato
                {
                    auto it = std::find(m_alivePlayers.begin(), m_alivePlayers.end(), p->pseudo);
                    if (it != m_alivePlayers.end())
                    {
                        bool wasBombHolder = (p->pseudo == m_bombHolder);
                        m_alivePlayers.erase(it);

                        if (m_alivePlayers.size() <= 1)
                        {
                            EndGame(m_alivePlayers.empty() ? "Personne" : m_alivePlayers[0]);
                        }
                        else if (wasBombHolder)
                        {
                            PickRandomBombHolder();
                        }
                    }
                }
                else if (m_activeGameID == 0) // Juste Prix
                {
                    bool anyActive = false;
                    for (const auto& [peer, info] : server->GetPlayers())
                    {
                        if (info.playerState == EPlayerState::Playing)
                        {
                            anyActive = true;
                            break;
                        }
                    }
                    if (!anyActive)
                    {
                        EndGame("Personne");
                        std::cout << "[GAME] Juste Prix stoppe car plus de joueurs actifs." << std::endl;
                    }
                }
            }
        }
        // Ignore other transitions (server decides spectating, not client)
    });

    // --- GAME DATA (Juste Prix guesses) ---
    server->GetNetwork().OnPacket(OpCode::GameData, [this, server](GamePacket& rawPkt, ENetPeer* sender)
    {
        if (!m_gameRunning || m_activeGameID != 0)
            return;

        PacketGameData pkt;
        pkt.Deserialize(rawPkt);
        
        PlayerInfo* player = server->GetPlayerByPeer(sender);
        if (player) 
            player->lastPacketTime = std::chrono::steady_clock::now();

        int guess = pkt.Value;
        
        // Validate range
        if (guess < 0 || guess > 99)
            return;
        
        PacketGameData response;
        
        if (guess < m_mysteryNumber)
        {
            response.Value = static_cast<int>(EGameDataType::JustePrixHintUp);
            server->SendTo(sender, response);
        }
        else if (guess > m_mysteryNumber)
        {
            response.Value = static_cast<int>(EGameDataType::JustePrixHintDown);
            server->SendTo(sender, response);
        }
        else
        {
            EndGame(player ? player->pseudo : "Unknown");
        }
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
              m_gameRunning = true;
              m_mysteryNumber = m_dist(m_rng);
              PacketGameStart startPkt;
              server->Broadcast(startPkt);
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

// ============================================================
// UPDATE — Hot Potato tick
// ============================================================
void MiniGameSystem::Update(float dt)
{
    if (!m_gameRunning || m_activeGameID != 1)
        return;

    // Tick bomb timer
    m_bombTimer -= dt;

    // Broadcast bomb state at ~10Hz
    m_bombBroadcastTimer += dt;
    if (m_bombBroadcastTimer >= 0.1f)
    {
        m_bombBroadcastTimer = 0.f;
        BroadcastBombState();
    }

    // Timer expired — eliminate bomb holder
    if (m_bombTimer <= 0.f)
    {
        EliminateBombHolder();
    }
}

// ============================================================
// OnPlayerDisconnect — remove from alive list
// ============================================================
void MiniGameSystem::OnPlayerDisconnect(PlayerInfo* player)
{
    if (!m_gameRunning || !player)
        return;

    if (m_activeGameID == 1) // Hot Potato
    {
        auto it = std::find(m_alivePlayers.begin(), m_alivePlayers.end(), player->pseudo);
        if (it == m_alivePlayers.end())
            return;

        bool wasBombHolder = (player->pseudo == m_bombHolder);
        m_alivePlayers.erase(it);

        std::cout << "[BOMB] " << player->pseudo << " deconnecte, retire des vivants." << std::endl;

        if (m_alivePlayers.size() <= 1)
        {
            EndGame(m_alivePlayers.empty() ? "Personne" : m_alivePlayers[0]);
            return;
        }

        if (wasBombHolder)
            PickRandomBombHolder();
    }
    else if (m_activeGameID == 0) // Juste Prix
    {
        // Check if any Playing players remain (excluding the one disconnecting)
        bool anyActive = false;
        for (const auto& [peer, info] : m_server->GetPlayers())
        {
            if (info.pseudo == player->pseudo) continue;
            if (info.playerState == EPlayerState::Playing)
            {
                anyActive = true;
                break;
            }
        }
        if (!anyActive)
        {
            EndGame("Personne");
            std::cout << "[GAME] Juste Prix stoppe: plus de joueurs actifs." << std::endl;
        }
    }
}

// ============================================================
// HandlePushHit — transfer bomb on push
// ============================================================
void MiniGameSystem::HandlePushHit(const std::string& pusher, const std::string& target)
{
    if (!m_gameRunning || m_activeGameID != 1)
        return;

    // Only transfer if the pusher IS the bomb holder
    if (pusher != m_bombHolder)
        return;

    // Only transfer to alive players
    auto it = std::find(m_alivePlayers.begin(), m_alivePlayers.end(), target);
    if (it == m_alivePlayers.end())
        return;

    m_bombHolder = target;
    std::cout << "[BOMB] Bombe transferee de " << pusher << " a " << target << std::endl;

    // Broadcast chat
    PacketChat msg;
    msg.Sender = "SYSTEM";
    msg.Message = pusher + " a passe la bombe a " + target + " !";
    msg.ChannelName = "Global";
    m_server->Broadcast(msg);

    // Immediate state update
    BroadcastBombState();
}

// ============================================================
// StartHotPotato
// ============================================================
void MiniGameSystem::StartHotPotato(GameServer* server)
{
    m_activeGameID = 1;
    m_gameRunning = true;
    m_bombBroadcastTimer = 0.f;

    // Set only Lobby players to Playing and gather alive list
    m_alivePlayers.clear();
    for (auto& [peer, info] : server->GetPlayers())
    {
        if (info.playerState == EPlayerState::Lobby)
        {
            SetPlayerState(info, EPlayerState::Playing);
            m_alivePlayers.push_back(info.pseudo);
        }
    }

    if (m_alivePlayers.size() < 2)
    {
        PacketChat msg;
        msg.Sender = "SYSTEM";
        msg.Message = "Il faut au moins 2 joueurs pour la Bombe !";
        msg.ChannelName = "System";
        server->Broadcast(msg);
        m_gameRunning = false;
        return;
    }

    // Pick random bomb holder
    PickRandomBombHolder();
    m_bombTimer = 10.f;

    std::cout << "[BOMB] Hot Potato demarre ! Porteur: " << m_bombHolder
              << " | " << m_alivePlayers.size() << " joueurs" << std::endl;

    // Broadcast game start
    PacketGameStart startPkt;
    startPkt.GameID = 1;
    server->Broadcast(startPkt);

    // Immediate state
    BroadcastBombState();
}

// ============================================================
// BroadcastBombState
// ============================================================
void MiniGameSystem::BroadcastBombState()
{
    PacketGameData pkt;
    pkt.Value     = static_cast<int>(EGameDataType::BombState);
    pkt.ExtraData = m_bombHolder;
    pkt.Timer     = m_bombTimer;
    m_server->Broadcast(pkt);
}

// ============================================================
// EliminateBombHolder
// ============================================================
void MiniGameSystem::EliminateBombHolder()
{
    std::string eliminated = m_bombHolder;
    std::cout << "[BOMB] " << eliminated << " ELIMINE !" << std::endl;

    // Notify elimination
    PacketGameData elimPkt;
    elimPkt.Value     = static_cast<int>(EGameDataType::BombElimination);
    elimPkt.ExtraData = eliminated;
    m_server->Broadcast(elimPkt);

    // Set victim as Spectator (server-authoritative)
    for (auto& [peer, info] : m_server->GetPlayers())
    {
        if (info.pseudo == eliminated)
        {
            SetPlayerState(info, EPlayerState::Spectating);
            break;
        }
    }

    // Chat message
    PacketChat msg;
    msg.Sender = "SYSTEM";
    msg.Message = eliminated + " a explose ! BOOOM !";
    msg.ChannelName = "Global";
    m_server->Broadcast(msg);

    // Remove from alive list
    auto it = std::find(m_alivePlayers.begin(), m_alivePlayers.end(), eliminated);
    if (it != m_alivePlayers.end())
        m_alivePlayers.erase(it);

    // Check win condition
    if (m_alivePlayers.size() <= 1)
    {
        EndGame(m_alivePlayers.empty() ? "Personne" : m_alivePlayers[0]);
        return;
    }

    // New round: pick new holder, reset timer
    PickRandomBombHolder();
    m_bombTimer = 10.f;
    BroadcastBombState();
}

// ============================================================
// PickRandomBombHolder
// ============================================================
void MiniGameSystem::PickRandomBombHolder()
{
    if (m_alivePlayers.empty())
        return;

    std::uniform_int_distribution<size_t> dist(0, m_alivePlayers.size() - 1);
    m_bombHolder = m_alivePlayers[dist(m_rng)];
}

void MiniGameSystem::OnPlayerConnect(PlayerInfo* player)
{
    // 1. Send game state to newcomer
    PacketGameData statePkt;
    statePkt.Value     = static_cast<int>(EGameDataType::GameStateChanged);
    statePkt.ExtraData = m_gameRunning ? "1" : "0";
    statePkt.Timer     = static_cast<float>(m_activeGameID);
    m_server->SendTo(player->peer, statePkt);

    // 2. Broadcast newcomer's state to all (so in-game players can hide them)
    PacketPlayerState newPkt;
    newPkt.Pseudo = player->pseudo;
    newPkt.State  = player->playerState;
    m_server->Broadcast(newPkt);

    // 3. Send all existing players' states to the newcomer (so they can hide in-game players)
    for (const auto& [peer, info] : m_server->GetPlayers())
    {
        if (info.pseudo == player->pseudo) continue;
        PacketPlayerState existPkt;
        existPkt.Pseudo = info.pseudo;
        existPkt.State  = info.playerState;
        m_server->SendTo(player->peer, existPkt);
    }
}
