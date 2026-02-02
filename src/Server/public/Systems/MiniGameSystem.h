#pragma once
#include <random>
#include "IServerSystem.h"


class MiniGameSystem : public IServerSystem
{
public:
    MiniGameSystem();
    void Init(GameServer* server) override;

private:
    bool m_gameRunning;
    int m_mysteryNumber;
    
    std::mt19937 m_rng;
    std::uniform_int_distribution<int> m_dist;
};
