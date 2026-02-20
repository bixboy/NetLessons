#include "MiniGames/MiniGameHotPotatoServer.h"
#include "Core/GameServer.h"
#include "Systems/MiniGameSystem.h"
#include <iostream>
#include <algorithm>

void MiniGameHotPotatoServer::Start(GameServer* server, MiniGameSystem* system)
{
    m_server = server;
    m_system = system;
    m_bombBroadcastTimer = 0.f;

    int lobbyPlayerCount = 0;
    for (const auto& [peer, info] : server->GetPlayers())
    {
        if (info.playerState == EPlayerState::Lobby)
        {
            lobbyPlayerCount++;
        }
    }

    if (lobbyPlayerCount < 2)
    {
        PacketChat msg;
        msg.Sender = "SYSTEM";
        msg.Message = "Il faut au moins 2 joueurs pour la Bombe !";
        msg.ChannelName = "System";
        server->Broadcast(msg);
        m_system->EndGame("Personne");
        return;
    }

    PacketGameStart startPkt;
    startPkt.GameID = 1;
    server->Broadcast(startPkt);

    m_system->m_alivePlayers.clear();
    for (auto& [peer, info] : server->GetPlayers())
    {
        if (info.playerState == EPlayerState::Lobby)
        {
            m_system->SetPlayerState(info, EPlayerState::Playing);
            m_system->m_alivePlayers.push_back(info.pseudo);
        }
    }

    PickRandomBombHolder();
    m_bombTimer = 10.f;

    std::cout << "[BOMB] Hot Potato demarre ! Porteur: " << m_bombHolder
              << " | " << m_system->m_alivePlayers.size() << " joueurs" << std::endl;

    BroadcastBombState();
}

void MiniGameHotPotatoServer::Update(float dt)
{
    // Tick bomb timer
    m_bombTimer -= dt;
    
    if (m_passCooldownTimer > 0.f)
        m_passCooldownTimer -= dt;

    m_bombBroadcastTimer += dt;
    if (m_bombBroadcastTimer >= 0.1f)
    {
        m_bombBroadcastTimer = 0.f;
        BroadcastBombState();
    }

    // Timer expired
    if (m_bombTimer <= 0.f)
    {
        EliminateBombHolder();
    }
}

void MiniGameHotPotatoServer::OnPlayerDisconnect(PlayerInfo* player)
{
    auto& alive = m_system->m_alivePlayers;
    auto it = std::find(alive.begin(), alive.end(), player->pseudo);
    if (it == alive.end())
        return;

    bool wasBombHolder = (player->pseudo == m_bombHolder);
    alive.erase(it);

    std::cout << "[BOMB] " << player->pseudo << " deconnecte, retire des vivants." << std::endl;

    if (alive.size() <= 1)
    {
        m_system->EndGame(alive.empty() ? "Personne" : alive[0]);
        return;
    }

    if (wasBombHolder)
        PickRandomBombHolder();
}

void MiniGameHotPotatoServer::OnPush(const std::string& pusher, const std::string& target)
{
    if (pusher != m_bombHolder)
        return;

    auto& alive = m_system->m_alivePlayers;
    auto it = std::find(alive.begin(), alive.end(), target);
    if (it == alive.end())
        return;

    // Cooldown check (Global preventing spam)
    if (m_passCooldownTimer > 0.f)
    {
         std::cout << "[BOMB] Pass ignored (cooldown)" << std::endl;
         return;
    }

    m_bombHolder = target;
    std::cout << "[BOMB] Bombe transferee de " << pusher << " a " << target << std::endl;
    
    // --- TIME BONUS & COOLDOWN ---
    m_bombTimer += 1.5f; 
    m_passCooldownTimer = 1.0f;

    PacketChat msg;
    msg.Sender = "SYSTEM";
    msg.Message = pusher + " a passe la bombe a " + target + " ! (+1.5s)";
    msg.ChannelName = "Global";
    m_server->Broadcast(msg);

    BroadcastBombState();
}

void MiniGameHotPotatoServer::PickRandomBombHolder()
{
    auto& alive = m_system->m_alivePlayers;
    if (alive.empty())
        return;

    std::uniform_int_distribution<size_t> dist(0, alive.size() - 1);
    m_bombHolder = alive[dist(m_system->m_rng)];
}

void MiniGameHotPotatoServer::EliminateBombHolder()
{
    std::string eliminated = m_bombHolder;
    std::cout << "[BOMB] " << eliminated << " ELIMINE !" << std::endl;

    PacketGameData elimPkt;
    elimPkt.Value     = static_cast<int>(EGameDataType::BombElimination);
    elimPkt.ExtraData = eliminated;
    m_server->Broadcast(elimPkt);

    for (auto& [peer, info] : m_server->GetPlayers())
    {
        if (info.pseudo == eliminated)
        {
            m_system->SetPlayerState(info, EPlayerState::Dead);
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
    auto& alive = m_system->m_alivePlayers;
    auto it = std::find(alive.begin(), alive.end(), eliminated);
    if (it != alive.end())
        alive.erase(it);

    // Check win condition
    if (alive.size() <= 1)
    {
        m_system->EndGame(alive.empty() ? "Personne" : alive[0]);
        return;
    }

    // New round
    PickRandomBombHolder();
    m_bombTimer = 10.f;
    BroadcastBombState();
}

void MiniGameHotPotatoServer::BroadcastBombState()
{
    PacketGameData pkt;
    pkt.Value = static_cast<int>(EGameDataType::BombState);
    pkt.ExtraData = m_bombHolder;
    pkt.Timer = m_bombTimer;
    
    m_server->Broadcast(pkt);
}
