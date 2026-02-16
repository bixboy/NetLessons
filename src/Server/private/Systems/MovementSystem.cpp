#include "Systems/MovementSystem.h"
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
    // Physics constants (normalized space)
    constexpr float FRICTION = 3.0f;
    constexpr float PUSH_FORCE = 0.8f;
    constexpr float PUSH_RADIUS = 0.15f;
    constexpr float PLAYER_RADIUS = 0.025f; // Approx 20px / 800px
    constexpr float PUSH_COOLDOWN = 1.0f;

    auto& players = m_server->GetPlayers();

    // 1. Apply Pushes & Cooldowns
    for (auto& [peer, info] : players)
    {
        if (info.playerState == EPlayerState::Spectating) continue;

        if (info.pushCooldown > 0.f)
            info.pushCooldown -= dt;

        if (info.requestPush && info.pushCooldown <= 0.f)
        {
            // Perform Push
            info.pushCooldown = PUSH_COOLDOWN;

            // Broadcast Action
            PacketPlayerAction actionPkt;
            actionPkt.Pseudo = info.pseudo;
            actionPkt.ActionType = 0; // Push
            m_server->Broadcast(actionPkt);

            // Apply knockback to nearby players
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

                    // Notify systems (bomb transfer, etc.)
                    if (m_server->OnPushHit)
                        m_server->OnPushHit(info.pseudo, targetInfo.pseudo);
                }
            }
        }
    }

    // 2. Movement & Physics Integration
    for (auto& [peer, info] : players)
    {
        // Input Movement
        float dx = static_cast<float>(info.inputDirX);
        float dy = static_cast<float>(info.inputDirY);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.f)
        {
            dx /= len;
            dy /= len;
        }

        // Apply Velocity (Input + Physics)
        info.posX += (dx * MOVE_SPEED + info.velocityX) * dt;
        info.posY += (dy * MOVE_SPEED + info.velocityY) * dt;

        // Friction
        float frictionFactor = 1.f / (1.f + (FRICTION * dt));
        info.velocityX *= frictionFactor;
        info.velocityY *= frictionFactor;

        // Clamp to normalized bounds [0, 1]
        info.posX = std::clamp(info.posX, BOUNDS_MIN, BOUNDS_MAX);
        info.posY = std::clamp(info.posY, BOUNDS_MIN, BOUNDS_MAX);
    }

    // 3. Collision Resolution (Simple iterations)
    // Run multiple passes for stability
    for (int i = 0; i < 4; ++i)
    {
        for (auto it1 = players.begin(); it1 != players.end(); ++it1)
        {
            PlayerInfo& p1 = it1->second;
            for (auto it2 = std::next(it1); it2 != players.end(); ++it2)
            {
                PlayerInfo& p2 = it2->second;

                if (p1.playerState == EPlayerState::Spectating || p2.playerState == EPlayerState::Spectating) continue;
                if (p1.playerState != p2.playerState) continue; // Different worlds don't collide

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

                    // Push apart
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

    // Broadcast positions at fixed rate
    m_broadcastTimer += dt;
    if (m_broadcastTimer >= BROADCAST_INTERVAL)
    {
        m_broadcastTimer = 0.f;

        for (const auto& [peer, info] : players)
        {
            if (info.playerState == EPlayerState::Spectating) continue;

            PacketPlayerPosition posPkt;
            posPkt.Pseudo = info.pseudo;
            posPkt.X = info.posX;
            posPkt.Y = info.posY;

            // Only send to players in the same state (Lobby sees Lobby, Playing sees Playing)
            for (const auto& [recvPeer, recvInfo] : players)
            {
                if (recvInfo.playerState == info.playerState)
                    m_server->SendTo(recvPeer, posPkt);
            }
        }
    }
}
