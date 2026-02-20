#pragma once
#include "MiniGames/IMiniGame.h"

class MiniGameRedLightGreenLight : public IMiniGame
{
public:
    MiniGameRedLightGreenLight(ClientContext& ctx) {}
    void Draw(ClientContext& ctx, UIRenderer& ui) override;
};
