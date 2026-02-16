#pragma once
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include "Avatars/AvatarManager.h"
#include "UI/ChatBox.h"
#include "Screens/ScreenIpConfig.h"
#include "Screens/ScreenLogin.h"
#include "Screens/ScreenLobby.h"
#include "Screens/ScreenGame.h"
#include "Screens/ScreenResult.h"


class GameClient
{
public:
    GameClient();
    ~GameClient();

    void Run();

private:
    void SetupNetworkHandlers();
    void UpdateLayout();

    // --- Core Systems ---
    ClientContext m_ctx;
    UIRenderer m_ui;
    AvatarManager m_avatars;
    ChatBox m_chat;

    // --- Screens ---
    ScreenIpConfig m_screenIpConfig;
    ScreenLogin m_screenLogin;
    ScreenLobby m_screenLobby;
    ScreenGame m_screenGame;
    ScreenResult m_screenResult;

    // --- Clocks ---
    sf::Clock m_dtClock;
};