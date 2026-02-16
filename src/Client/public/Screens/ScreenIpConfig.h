#pragma once
#include "Screens/IScreen.h"

class ClientContext;
class UIRenderer;


class ScreenIpConfig : public IScreen
{
public:
    ScreenIpConfig(ClientContext& ctx, UIRenderer& ui);
    void HandleInput(const sf::Event& event) override;
    void Draw() override;

private:
    ClientContext& m_ctx;
    UIRenderer& m_ui;
};
