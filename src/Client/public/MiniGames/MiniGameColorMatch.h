#pragma once
#include "IMiniGame.h"
#include <vector>
#include <SFML/Graphics.hpp>

class MiniGameColorMatch : public IMiniGame
{
public:
    MiniGameColorMatch(ClientContext& ctx);
    
    void Init() override;
    void Update(float dt) override;
    void Draw(ClientContext& ctx, UIRenderer& ui) override;
    void OnPacket(const PacketGameData& pkt) override;
    
private:
    ClientContext& m_ctx;
    std::vector<int> m_gridColors;
    int m_targetColor = -1;
    float m_stateTimer = 0.f;
    
    enum class EPhase { Warmup, Reset, Tension, Reveal, Elimination };
    EPhase m_phase = EPhase::Warmup;
    
    float m_visualTimer = 0.f;
    
    // Config
    static constexpr int GRID_W = 4;
    static constexpr int GRID_H = 4;
    
    sf::Color GetColorFromID(int id);
    
    struct Particle {
        sf::Vector2f pos;
        sf::Vector2f vel;
        sf::Color color;
        float life;
    };
    std::vector<Particle> m_particles;
    void SpawnParticles(sf::Vector2f pos, sf::Color color, int count);
    
    float m_shakeTimer = 0.f;
    float m_shakeMagnitude = 0.f;
};
