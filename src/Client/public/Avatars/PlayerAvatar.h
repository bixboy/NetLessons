#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <vector>


struct InteractionZone
{
    sf::FloatRect bounds;
    std::string label;
    std::function<void()> action;
    float holdTime = 3.f;   // Seconds to hold E before triggering (default 3s)
    bool repeatable = false; // If true, re-triggers after each holdTime while held
};


class PlayerAvatar
{
public:
    PlayerAvatar() = default;
    PlayerAvatar(const std::string& pseudo, sf::Color color, bool isLocal = false);

    // Server sends normalized positions [0,1], client converts to screen
    void SetTargetNormalized(float nx, float ny);
    void SetPositionNormalized(float nx, float ny);
    void Update(float dt, bool canInput = true);
    void Draw(sf::RenderWindow& window, sf::Font& font);

    // Push feedback
    void TriggerPushEffect();
    void TriggerHitEffect();
    bool IsPushing() const { return m_pushCooldown > 0.f; }
    
    sf::Vector2f GetPosition() const { return m_screenPos; }
    sf::FloatRect GetBounds() const;
    
    const std::string& GetPseudo() const { return m_pseudo; }
    void SetColor(sf::Color color) { m_color = color; }
    bool IsLocal() const { return m_isLocal; }
    void SetBombHolder(bool isHolder) { m_isBombHolder = isHolder; }
    void SetSpectator(bool isSpec) { m_isSpectator = isSpec; }

    // Call this when window resizes — defines the play area
    void SetPlayArea(float marginLeft, float marginTop, float areaWidth, float areaHeight);

private:
    // Conversion helpers
    sf::Vector2f NormalizedToScreen(float nx, float ny) const;

    std::string m_pseudo;
    sf::Color m_color = sf::Color::White;
    bool m_isLocal = false;
    bool m_isBombHolder = false;
    bool m_isSpectator = false;
    
    // Screen-space position and velocity
    sf::Vector2f m_screenPos = {450.f, 350.f};
    sf::Vector2f m_velocity = {0.f, 0.f};
    
    // Normalized target from server [0,1]
    sf::Vector2f m_targetNorm = {0.5f, 0.5f};
    
    // Play area mapping
    float m_areaX = 20.f;
    float m_areaY = 20.f;
    float m_areaW = 860.f;
    float m_areaH = 530.f;
    
    // Push animation state
    float m_pushScale = 1.f;       // 1.0 = normal, peaks at 1.4
    float m_flashAlpha = 0.f;      // White flash overlay (0-255)
    float m_shockwaveRadius = 0.f; // Expanding ring radius
    float m_shockwaveAlpha = 0.f;  // Ring opacity
    float m_pushCooldown = 0.f;    // Cooldown timer (1s)
    sf::Color m_hitFlashColor = sf::Color::White;
    
    static constexpr float AVATAR_RADIUS = 16.f;
    static constexpr float INTERP_SPEED = 15.f;
    static constexpr float NORM_SPEED = 0.25f;    // Must match server
    static constexpr float PUSH_COOLDOWN_TIME = 1.0f;
    static constexpr float ACCEL = 8.f;            // Acceleration factor
    static constexpr float DECEL = 10.f;           // Deceleration (friction)
    static constexpr float CORRECTION_STRENGTH = 8.f; // Server correction lerp
};
