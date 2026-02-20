#pragma once
#include "MiniGames/IMiniGame.h"

class MiniGameHotPotato : public IMiniGame
{
public:
    MiniGameHotPotato(ClientContext& ctx) : m_ctx(ctx) {}
    void Draw(ClientContext& ctx, UIRenderer& ui) override;
    void OnPacket(const PacketGameData& pkt) override;

private:
   ClientContext& m_ctx;
};
