#pragma once
#include "Screens/IScreen.h"

class ClientContext;
class UIRenderer;


class ScreenResult : public IScreen
{
public:
    ScreenResult(ClientContext& ctx, UIRenderer& ui);
    void HandleInput(const sf::Event& event) override;
    void Draw() override;

private:
    ClientContext& m_ctx;
    UIRenderer& m_ui;
};
