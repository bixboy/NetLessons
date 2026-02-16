#pragma once
#include "IServerSystem.h"


class MovementSystem : public IServerSystem
{
public:
    void Init(GameServer* server) override;
    void Update(float dt) override;

private:
    GameServer* m_server = nullptr;

    static constexpr float MOVE_SPEED = 0.25f;
    static constexpr float BOUNDS_MIN = 0.f;
    static constexpr float BOUNDS_MAX = 1.f;

    float m_broadcastTimer = 0.f;
    static constexpr float BROADCAST_INTERVAL = 0.1f;
};
