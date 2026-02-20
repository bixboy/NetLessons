#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <vector>
#include "PacketSystem.h"


struct InteractionZone
{
    sf::FloatRect bounds;
    std::string label;
    std::function<void()> action;
    float holdTime = 3.f;
    bool repeatable = false;
};


class PlayerAvatar
{
public:
    PlayerAvatar() = default;
    PlayerAvatar(const std::string& pseudo, sf::Color color, bool isLocal = false);

    void SetTargetNormalized(float nx, float ny);
    void SetPositionNormalized(float nx, float ny);
    void Update(float dt, bool canInput = true);
    void Draw(sf::RenderTarget& target, sf::Font& font);

    // Push feedback
    void TriggerPushEffect();
    void TriggerHitEffect();
    void TriggerExplosionEffect();
    bool IsPushing() const { return m_pushCooldown > 0.f; }
    
    sf::Vector2f GetPosition() const { return m_screenPos; }
    sf::FloatRect GetBounds() const;
    
    const std::string& GetPseudo() const { return m_pseudo; }
    void SetColor(sf::Color color) { m_color = color; }
    bool IsLocal() const { return m_isLocal; }
    void SetBombHolder(bool isHolder) { m_isBombHolder = isHolder; }
    void SetSpectator(bool isSpec) { m_isSpectator = isSpec; }
    
    void SetState(EPlayerState state) { m_state = state; }
    EPlayerState GetState() const { return m_state; }

    void SetPlayArea(float marginLeft, float marginTop, float areaWidth, float areaHeight);

private:
    sf::Vector2f NormalizedToScreen(float nx, float ny) const;

    std::string m_pseudo;
    sf::Color m_color = sf::Color::White;
    bool m_isLocal = false;
    bool m_isBombHolder = false;
    bool m_isSpectator = false;
    EPlayerState m_state = EPlayerState::Lobby;
    bool m_isIceMode = false;
    
public:
    void SetIceMode(bool ice) { m_isIceMode = ice; }
    
    sf::Vector2f m_screenPos = {450.f, 350.f};
    sf::Vector2f m_velocity = {0.f, 0.f};
    
    sf::Vector2f m_targetNorm = {0.5f, 0.5f};
    
    // Play area mapping
    float m_areaX = 20.f;
    float m_areaY = 20.f;
    float m_areaW = 860.f;
    float m_areaH = 530.f;
    
    // Push animation state
    float m_pushScale = 1.f;
    float m_flashAlpha = 0.f;
    float m_shockwaveRadius = 0.f;
    float m_shockwaveAlpha = 0.f;

    // Explosion Effect
    float m_explosionRadius = 0.f;
    float m_explosionAlpha = 0.f;
    float m_pushCooldown = 0.f;
    sf::Color m_hitFlashColor = sf::Color::White;
    
    // --- GORE PARTICLES ---
    struct Particle
    {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life = 0.f;
        float maxLife = 1.f;
        float size = 2.f;
        sf::Color color;
        float drag = 0.f;
    };
    std::vector<Particle> m_particles;
    void SpawnGore();
    
    static constexpr float AVATAR_RADIUS = 16.f;
    static constexpr float INTERP_SPEED = 15.f;
    static constexpr float NORM_SPEED = 0.25f;
    static constexpr float PUSH_COOLDOWN_TIME = 1.0f;
    static constexpr float ACCEL = 8.f;
    static constexpr float DECEL = 10.f;
    static constexpr float CORRECTION_STRENGTH = 8.f;
};
