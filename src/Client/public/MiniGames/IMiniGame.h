#pragma once


class ClientContext;
class UIRenderer;

#include "PacketSystem.h"

class IMiniGame
{
public:
    virtual ~IMiniGame() = default;

    virtual void Init() {}
    virtual void Draw(ClientContext& ctx, UIRenderer& ui) = 0;
    virtual void Update(float dt) {}
    virtual void OnPacket(const PacketGameData& pkt) {}
};
