#pragma once
#include "MiniGames/IMiniGame.h"

class MiniGameJustePrix : public IMiniGame
{
public:
    MiniGameJustePrix(ClientContext& ctx) {}
    void Draw(ClientContext& ctx, UIRenderer& ui) override;
};
