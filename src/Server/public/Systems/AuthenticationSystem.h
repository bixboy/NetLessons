#pragma once
#include "IServerSystem.h"
#include <random>


class AuthenticationSystem : public IServerSystem
{
public:
    void Init(GameServer* server) override;
    void Update(float dt) override;

private:
    GameServer* m_server = nullptr;
    std::mt19937 m_rng{std::random_device{}()};
    std::uniform_int_distribution<int> m_colorDist{0, 7};
};
