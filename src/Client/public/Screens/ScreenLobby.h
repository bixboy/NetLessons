#pragma once
#include "Screens/IScreen.h"

class ClientContext;
class UIRenderer;
class AvatarManager;


class ScreenLobby : public IScreen
{
public:
    ScreenLobby(ClientContext& ctx, UIRenderer& ui, AvatarManager& avatars);
    void HandleInput(const sf::Event& event) override;
    void Draw() override;

private:
    ClientContext& m_ctx;
    UIRenderer& m_ui;
    AvatarManager& m_avatars;
};
