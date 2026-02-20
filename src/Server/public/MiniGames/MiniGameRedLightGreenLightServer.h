#pragma once
#include "IServerMiniGame.h"


class MiniGameRedLightGreenLightServer : public IServerMiniGame
{
public:
    void Start(GameServer* server, MiniGameSystem* system) override;
    void Update(float dt) override;
    void OnPlayerDisconnect(PlayerInfo* player) override;
    
    bool IsIceMode() const override { return m_isIceMode; }

private:
   
    struct RedLightGreenLightState
    {
        bool IsRedLight = false;
        float Timer = 0.f;
        float GracePeriod = 0.0f;

        static constexpr float FINISH_LINE_X = 0.9f;
        static constexpr float START_LINE_X = 0.1f;
        static constexpr float MOVEMENT_THRESHOLD = 0.005f;
    };

    GameServer* m_server = nullptr;
    MiniGameSystem* m_system = nullptr;

    RedLightGreenLightState m_rlgl;
    bool m_isIceMode = false;
};
