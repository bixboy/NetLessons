#pragma once
#include "IServerSystem.h"

class AuthenticationSystem : public IServerSystem
{
public:
    void Init(GameServer* server) override;
    void Update(float dt) override;

private:
    GameServer* server = nullptr;
};
