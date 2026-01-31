#pragma once
#include "IServerSystem.h"

class ChatSystem : public IServerSystem
{
public:
    void Init(GameServer* server) override;
};
