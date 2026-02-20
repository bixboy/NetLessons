#include "Systems/MovementSystem.h"
#include "Systems/MiniGameSystem.h"
#include "Core/GameServer.h"

#include "PacketSystem.h"

#include <iostream>
#include <algorithm>
#include <cmath>


void MovementSystem::Init(GameServer* server)
{
    m_server = server;

    m_server->GetNetwork().OnPacket(OpCode::PlayerInput,
    [this](GamePacket& rawPkt, ENetPeer* sender)
    {
        PacketPlayerInput pkt;
        pkt.Deserialize(rawPkt);

        PlayerInfo* player = m_server->GetPlayerByPeer(sender);
        if (!player)
            return;

        player->inputDirX = std::clamp(pkt.DirX, static_cast<int8_t>(-1), static_cast<int8_t>(1));
        player->inputDirY = std::clamp(pkt.DirY, static_cast<int8_t>(-1), static_cast<int8_t>(1));
        player->requestPush = pkt.Push;

        player->lastPacketTime = std::chrono::steady_clock::now();
    });
}

void MovementSystem::Update(float dt)
{
    constexpr float NORMAL_FRICTION = 3.0f;
    constexpr float ICE_FRICTION = 0.05f;

    float frictionVal = NORMAL_FRICTION;
    
    if (MiniGameSystem::s_instance && MiniGameSystem::s_instance->IsIceMode())
    {
        frictionVal = ICE_FRICTION;
    }
    
    constexpr float PUSH_FORCE = 0.8f;
    constexpr float PUSH_RADIUS = 0.15f;
    constexpr float PLAYER_RADIUS = 0.025f;
    constexpr float PUSH_COOLDOWN = 1.0f;

    auto& players = m_server->GetPlayers();

    for (auto& [peer, info] : players)
    {
        if (info.playerState == EPlayerState::Spectating)
            continue;

        if (info.pushCooldown > 0.f)
            info.pushCooldown -= dt;

        if (info.requestPush && info.pushCooldown <= 0.f)
        {
            info.pushCooldown = PUSH_COOLDOWN;

            PacketPlayerAction actionPkt;
            actionPkt.Pseudo = info.pseudo;
            actionPkt.ActionType = 0;
            m_server->Broadcast(actionPkt);

            for (auto& [targetPeer, targetInfo] : players)
            {
                if (peer == targetPeer || targetInfo.playerState == EPlayerState::Spectating) 
                    continue;

                if (targetInfo.playerState != info.playerState)
                    continue;

                float dx = targetInfo.posX - info.posX;
                float dy = targetInfo.posY - info.posY;
                float distSq = dx*dx + dy*dy;

                if (distSq < PUSH_RADIUS*PUSH_RADIUS)
                {
                    float dist = std::sqrt(distSq);
                    float nx = (dist > 0.0001f) ? dx / dist : 1.f;
                    float ny = (dist > 0.0001f) ? dy / dist : 0.f;


                    targetInfo.velocityX += nx * PUSH_FORCE;
                    targetInfo.velocityY += ny * PUSH_FORCE;
                    
                    constexpr float WALL_MARGIN = 0.05f;
                    bool isPinnedX = (targetInfo.posX < BOUNDS_MIN + WALL_MARGIN && nx < 0) || 
                                     (targetInfo.posX > BOUNDS_MAX - WALL_MARGIN && nx > 0);
                    
                    bool isPinnedY = (targetInfo.posY < BOUNDS_MIN + WALL_MARGIN && ny < 0) || 
                                     (targetInfo.posY > BOUNDS_MAX - WALL_MARGIN && ny > 0);
                                     
                    if (isPinnedX || isPinnedY)
                    {
                        targetInfo.wallPinCount++;
                        
                        if (targetInfo.wallPinCount >= 3)
                        {
                            targetInfo.playerState = EPlayerState::Dead;
                            targetInfo.respawnTimer = 5.0f;
                            targetInfo.wallPinCount = 0;
                            
                            PacketPlayerAction explPkt;
                            explPkt.Pseudo = targetInfo.pseudo;
                            explPkt.ActionType = 2;
                            m_server->Broadcast(explPkt);
                            
                            PacketGameData timerPkt;
                            timerPkt.Value = static_cast<int>(EGameDataType::RespawnTimer);
                            timerPkt.Timer = targetInfo.respawnTimer;
                            m_server->SendTo(targetPeer, timerPkt);
                            
                            PacketPlayerState statePkt;
                            statePkt.Pseudo = targetInfo.pseudo;
                            statePkt.State = EPlayerState::Dead;
                            m_server->Broadcast(statePkt);
                            
                            std::cout << "[DEATH] " << targetInfo.pseudo << " exploded against a wall!" << std::endl;
                        }
                    }
                    else
                    {
                        targetInfo.wallPinCount = 0;
                    }

                    if (m_server->OnPushHit)
                        m_server->OnPushHit(info.pseudo, targetInfo.pseudo);
                }
            }
        }
    }

    for (auto& [peer, info] : players)
    {
        // Input Movement
        float dx = info.inputDirX;
        float dy = info.inputDirY;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.f)
        {
            dx /= len;
            dy /= len;
        }

        bool isIce = (MiniGameSystem::s_instance && MiniGameSystem::s_instance->IsIceMode());
        if (isIce)
        {
            // --- ICE PHYSICS ---
            constexpr float ICE_ACCEL = 1.2f;
            constexpr float ICE_DRAG = 2.0f;
            constexpr float MAX_ICE_SPEED = 0.5f;

            info.velocityX += dx * ICE_ACCEL * dt;
            info.velocityY += dy * ICE_ACCEL * dt;

            float dragFactor = 1.0f - (ICE_DRAG * dt);
            if (dragFactor < 0.f)
                dragFactor = 0.f;
            
            info.velocityX *= dragFactor;
            info.velocityY *= dragFactor;

            info.posX += info.velocityX * dt;
            info.posY += info.velocityY * dt;
            
            float speedSq = info.velocityX * info.velocityX + info.velocityY * info.velocityY;
            if (speedSq > MAX_ICE_SPEED * MAX_ICE_SPEED)
            {
                float speed = std::sqrt(speedSq);
                float scale = MAX_ICE_SPEED / speed;
                info.velocityX *= scale;
                info.velocityY *= scale;
            }
        }
        else
        {
            // --- NORMAL PHYSICS ---
            info.posX += (dx * MOVE_SPEED + info.velocityX) * dt;
            info.posY += (dy * MOVE_SPEED + info.velocityY) * dt;
            
            float frictionFactor = 1.f / (1.f + (frictionVal * dt));
            info.velocityX *= frictionFactor;
            info.velocityY *= frictionFactor;
        }


        info.posX = std::clamp(info.posX, BOUNDS_MIN, BOUNDS_MAX);
        info.posY = std::clamp(info.posY, BOUNDS_MIN, BOUNDS_MAX);
        
        if (info.playerState == EPlayerState::Dead && info.respawnTimer > 0.f)
        {
            info.respawnTimer -= dt;
            if (info.respawnTimer <= 0.f)
            {
                // RESPAWN
                info.playerState = EPlayerState::Lobby;
                info.posX = 0.5f;
                info.posY = 0.5f;
                info.velocityX = 0.f;
                info.velocityY = 0.f;
                info.wallPinCount = 0;
                
                PacketPlayerState statePkt;
                statePkt.Pseudo = info.pseudo;
                statePkt.State = EPlayerState::Lobby;
                m_server->Broadcast(statePkt);
                
                std::cout << "[RESPAWN] " << info.pseudo << " is back." << std::endl;
            }
        }
    }

    // 3. Collision Resolution
    for (int i = 0; i < 4; ++i)
    {
        for (auto it1 = players.begin(); it1 != players.end(); ++it1)
        {
            PlayerInfo& p1 = it1->second;
            for (auto it2 = std::next(it1); it2 != players.end(); ++it2)
            {
                PlayerInfo& p2 = it2->second;

                if (p1.playerState == EPlayerState::Spectating || p2.playerState == EPlayerState::Spectating)
                    continue;
                
                if (p1.playerState != p2.playerState)
                    continue;

                float dx = p2.posX - p1.posX;
                float dy = p2.posY - p1.posY;
                float distSq = dx*dx + dy*dy;
                float minDist = PLAYER_RADIUS * 2.f;

                if (distSq < minDist * minDist)
                {
                    float dist = std::sqrt(distSq);
                    float overlap = minDist - dist;
                    float nx = (dist > 0.0001f) ? dx / dist : 1.f;
                    float ny = (dist > 0.0001f) ? dy / dist : 0.f;

                    float moveX = nx * overlap * 0.5f;
                    float moveY = ny * overlap * 0.5f;

                    p1.posX -= moveX;
                    p1.posY -= moveY;
                    p2.posX += moveX;
                    p2.posY += moveY;
                }
            }
        }
    }

    m_broadcastTimer += dt;
    if (m_broadcastTimer >= BROADCAST_INTERVAL)
    {
        m_broadcastTimer = 0.f;

        for (const auto& [peer, info] : players)
        {
            if (info.playerState == EPlayerState::Spectating)
                continue;

            PacketPlayerPosition posPkt;
            posPkt.Pseudo = info.pseudo;
            posPkt.X = info.posX;
            posPkt.Y = info.posY;
            
            for (const auto& [recvPeer, recvInfo] : players)
            {
                bool sameState = (recvInfo.playerState == info.playerState);
                bool spectatorWatchingGame = (recvInfo.playerState == EPlayerState::Spectating && info.playerState == EPlayerState::Playing);
                
                bool deadSeeLobby = (recvInfo.playerState == EPlayerState::Dead && info.playerState == EPlayerState::Lobby);
                bool lobbySeeDead = (recvInfo.playerState == EPlayerState::Lobby && info.playerState == EPlayerState::Dead);
                bool deadSeeDead = (recvInfo.playerState == EPlayerState::Dead && info.playerState == EPlayerState::Dead);
                bool deadSeePlaying = (recvInfo.playerState == EPlayerState::Dead && info.playerState == EPlayerState::Playing);

                if (sameState || spectatorWatchingGame || deadSeeLobby || lobbySeeDead || deadSeeDead || deadSeePlaying)
                {
                    m_server->SendTo(recvPeer, posPkt);
                }
                else
                {
                    if (info.playerState == EPlayerState::Playing)
                    {
                        static int logCounter = 0;
                        if (logCounter++ % 200 == 0)
                        {
                             std::cout << "[VISIBILITY WARN] Hiding " << info.pseudo << " (Playing) from " << recvInfo.pseudo << " (State: " << (int)recvInfo.playerState << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }
}
