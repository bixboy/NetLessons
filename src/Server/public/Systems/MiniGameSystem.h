#pragma once
#include "IServerSystem.h"

class MiniGameSystem : public IServerSystem
{
public:
    MiniGameSystem();
    void Init(GameServer* server) override;

private:
    bool m_gameRunning;
    int m_mysteryNumber;
};
