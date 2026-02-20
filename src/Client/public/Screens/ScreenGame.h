#pragma once
#include "Screens/IScreen.h"
#include <memory>
#include "MiniGames/IMiniGame.h"

class ClientContext;
class UIRenderer;

class ScreenGame : public IScreen
{
public:
    ScreenGame(ClientContext& ctx, UIRenderer& ui);
    void HandleInput(const sf::Event& event) override;
    void Update(float dt) override;
    void Draw() override;
    void OnPacket(const PacketGameData& pkt);

private:
    void CheckGameSwitch();

private:
    ClientContext& m_ctx;
    UIRenderer& m_ui;
    
    std::unique_ptr<IMiniGame> m_currentGame;
    int m_lastGameID = -1;
};
