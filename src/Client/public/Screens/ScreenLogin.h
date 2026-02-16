#pragma once
#include "Screens/IScreen.h"

class ClientContext;
class UIRenderer;
class ChatBox;
class AvatarManager;


class ScreenLogin : public IScreen
{
public:
    ScreenLogin(ClientContext& ctx, UIRenderer& ui, ChatBox& chat, AvatarManager& avatars);
    void HandleInput(const sf::Event& event) override;
    void Draw() override;

private:
    ClientContext& m_ctx;
    UIRenderer& m_ui;
    ChatBox& m_chat;
    AvatarManager& m_avatars;
};
