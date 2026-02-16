#pragma once
#include "Screens/IScreen.h"

class ClientContext;
class UIRenderer;


class ScreenGame : public IScreen
{
public:
    ScreenGame(ClientContext& ctx, UIRenderer& ui);
    void HandleInput(const sf::Event& event) override;
    void Draw() override;

private:
    void DrawJustePrix();
    void DrawHotPotato();

    ClientContext& m_ctx;
    UIRenderer& m_ui;
};
