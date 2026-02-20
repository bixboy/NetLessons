#pragma once
#include "IServerMiniGame.h"
#include <vector>


class MiniGameColorMatchServer : public IServerMiniGame
{
public:
    void Start(GameServer* server, MiniGameSystem* system) override;
    void Update(float dt) override;
    void OnPlayerDisconnect(PlayerInfo* player) override;
    void OnPlayerConnect(PlayerInfo* player) override;
    
    bool IsIceMode() const override { return false; }

private:
    struct ColorMatchState
    {
        enum class EPhase { Warmup, Reset, Tension, Reveal, Elimination };
        EPhase Phase = EPhase::Warmup;
        
        std::vector<uint8_t> GridColors; 
        int TargetColor = 0;
        float Timer = 0.f;
        float RoundDuration = 6.0f;
    };

    GameServer* m_server = nullptr;
    MiniGameSystem* m_system = nullptr;
    ColorMatchState m_colorMatch;
};
