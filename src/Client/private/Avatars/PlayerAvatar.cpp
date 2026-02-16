#include "Avatars/PlayerAvatar.h"
#include <cmath>
#include <algorithm>

static constexpr float PI = 3.14159265f;


PlayerAvatar::PlayerAvatar(const std::string& pseudo, sf::Color color, bool isLocal)
    : m_pseudo(pseudo), m_color(color), m_isLocal(isLocal)
{
}

void PlayerAvatar::SetPlayArea(float marginLeft, float marginTop, float areaWidth, float areaHeight)
{
    bool changed = (m_areaX != marginLeft || m_areaY != marginTop || m_areaW != areaWidth || m_areaH != areaHeight);
    
    float oldAreaX = m_areaX, oldAreaY = m_areaY;
    float oldAreaW = m_areaW, oldAreaH = m_areaH;
    
    m_areaX = marginLeft;
    m_areaY = marginTop;
    m_areaW = areaWidth;
    m_areaH = areaHeight;
    
    if (changed && oldAreaW > 0.f && oldAreaH > 0.f)
    {
        float normX = (m_screenPos.x - oldAreaX) / oldAreaW;
        float normY = (m_screenPos.y - oldAreaY) / oldAreaH;
        m_screenPos = NormalizedToScreen(normX, normY);
    }
}

sf::Vector2f PlayerAvatar::NormalizedToScreen(float nx, float ny) const
{
    return {
        m_areaX + nx * m_areaW,
        m_areaY + ny * m_areaH
    };
}

void PlayerAvatar::SetTargetNormalized(float nx, float ny)
{
    m_targetNorm = {nx, ny};
}

void PlayerAvatar::SetPositionNormalized(float nx, float ny)
{
    m_targetNorm = {nx, ny};
    m_screenPos = NormalizedToScreen(nx, ny);
}

void PlayerAvatar::Update(float dt, bool canInput)
{
    if (m_isLocal)
    {
        float dirX = 0.f, dirY = 0.f;

        if (canInput)
        {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
                dirY = -1.f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
                dirY = 1.f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
                dirX = -1.f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
                dirX = 1.f;
        }

        float len = std::sqrt(dirX * dirX + dirY * dirY);
        if (len > 0.f)
        {
            dirX /= len;
            dirY /= len;
        }

        // Target velocity based on input
        float maxSpeed = NORM_SPEED * m_areaW;
        float targetVX = dirX * maxSpeed;
        float targetVY = dirY * NORM_SPEED * m_areaH;

        // Accelerate toward target velocity, decelerate via friction
        if (len > 0.f)
        {
            m_velocity.x += (targetVX - m_velocity.x) * ACCEL * dt;
            m_velocity.y += (targetVY - m_velocity.y) * ACCEL * dt;
        }
        else
        {
            m_velocity.x *= std::max(0.f, 1.f - DECEL * dt);
            m_velocity.y *= std::max(0.f, 1.f - DECEL * dt);

            // Kill tiny residual velocity
            if (std::abs(m_velocity.x) < 0.5f) m_velocity.x = 0.f;
            if (std::abs(m_velocity.y) < 0.5f) m_velocity.y = 0.f;
        }

        m_screenPos.x += m_velocity.x * dt;
        m_screenPos.y += m_velocity.y * dt;

        m_screenPos.x = std::clamp(m_screenPos.x, m_areaX, m_areaX + m_areaW);
        m_screenPos.y = std::clamp(m_screenPos.y, m_areaY, m_areaY + m_areaH);

        // Progressive server correction (smooth, no hard snap)
        sf::Vector2f targetScreen = NormalizedToScreen(
            std::clamp(m_targetNorm.x, 0.f, 1.f),
            std::clamp(m_targetNorm.y, 0.f, 1.f)
        );
        
        float diffX = targetScreen.x - m_screenPos.x;
        float diffY = targetScreen.y - m_screenPos.y;
        float dist = std::sqrt(diffX * diffX + diffY * diffY);

        if (dist > 2.f)
        {
            // Correction strength scales with distance — farther = stronger pull
            float t = std::min(1.f, CORRECTION_STRENGTH * dt * (dist / 50.f));
            m_screenPos.x += diffX * t;
            m_screenPos.y += diffY * t;
        }
    }
    else
    {
        sf::Vector2f targetScreen = NormalizedToScreen(m_targetNorm.x, m_targetNorm.y);
        float diffX = targetScreen.x - m_screenPos.x;
        float diffY = targetScreen.y - m_screenPos.y;

        m_screenPos.x += diffX * INTERP_SPEED * dt;
        m_screenPos.y += diffY * INTERP_SPEED * dt;
    }

    // Animate push effects
    if (m_pushScale > 1.f)
        m_pushScale = std::max(1.f, m_pushScale - 2.f * dt); // Shrink back over ~0.2s

    if (m_flashAlpha > 0.f)
        m_flashAlpha = std::max(0.f, m_flashAlpha - 600.f * dt); // Fade over ~0.4s

    if (m_shockwaveAlpha > 0.f)
    {
        m_shockwaveRadius += 200.f * dt;  // Expand ring
        m_shockwaveAlpha = std::max(0.f, m_shockwaveAlpha - 400.f * dt);
    }

    if (m_pushCooldown > 0.f)
        m_pushCooldown = std::max(0.f, m_pushCooldown - dt);
}

void PlayerAvatar::Draw(sf::RenderWindow& window, sf::Font& font)
{
    if (m_isSpectator)
        return; // Invisible

    float scaledRadius = AVATAR_RADIUS * m_pushScale;

    // Shockwave ring (expanding circle outline)
    if (m_shockwaveAlpha > 0.f)
    {
        sf::CircleShape ring(m_shockwaveRadius);
        ring.setOrigin({m_shockwaveRadius, m_shockwaveRadius});
        ring.setPosition(m_screenPos);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.f);
        ring.setOutlineColor(sf::Color(255, 255, 255, static_cast<uint8_t>(m_shockwaveAlpha)));
        ring.setPointCount(48);
        window.draw(ring);
    }

    // Shadow
    sf::CircleShape shadow(scaledRadius);
    shadow.setOrigin({scaledRadius, scaledRadius});
    shadow.setPosition({m_screenPos.x + 3.f, m_screenPos.y + 3.f});
    shadow.setFillColor(sf::Color(0, 0, 0, 80));
    window.draw(shadow);

    // Bomb holder glow (pulsing red ring)
    if (m_isBombHolder)
    {
        float glowRadius = scaledRadius + 8.f;
        sf::CircleShape glow(glowRadius);
        glow.setOrigin({glowRadius, glowRadius});
        glow.setPosition(m_screenPos);
        glow.setFillColor(sf::Color::Transparent);
        glow.setOutlineThickness(4.f);
        // Pulsing alpha
        float pulse = std::sin(m_pushCooldown * 10.f + scaledRadius) * 0.3f + 0.7f; // reuse any timer
        uint8_t alpha = static_cast<uint8_t>(200 * pulse);
        glow.setOutlineColor(sf::Color(255, 50, 30, alpha));
        glow.setPointCount(32);
        window.draw(glow);
    }

    // Body (scaled)
    sf::CircleShape body(scaledRadius);
    body.setOrigin({scaledRadius, scaledRadius});
    body.setPosition(m_screenPos);
    body.setFillColor(m_color);
    body.setOutlineThickness(2.f);
    if (m_isBombHolder)
        body.setOutlineColor(sf::Color(255, 60, 30));
    else
        body.setOutlineColor(m_isLocal ? sf::Color::White : sf::Color(100, 100, 100));
    window.draw(body);

    // Flash overlay (white/red circle on top)
    if (m_flashAlpha > 0.f)
    {
        sf::CircleShape flash(scaledRadius);
        flash.setOrigin({scaledRadius, scaledRadius});
        flash.setPosition(m_screenPos);
        flash.setFillColor(sf::Color(
            m_hitFlashColor.r, m_hitFlashColor.g, m_hitFlashColor.b,
            static_cast<uint8_t>(m_flashAlpha)
        ));
        window.draw(flash);
    }

    // Cooldown arc (local avatar only)
    if (m_isLocal && m_pushCooldown > 0.f)
    {
        float ratio = m_pushCooldown / PUSH_COOLDOWN_TIME;
        int segments = static_cast<int>(ratio * 24);
        if (segments > 0)
        {
            float arcRadius = AVATAR_RADIUS + 6.f;
            sf::VertexArray arc(sf::PrimitiveType::LineStrip, segments + 1);
            for (int i = 0; i <= segments; ++i)
            {
                float angle = -PI / 2.f + (static_cast<float>(i) / 24.f) * 2.f * PI;
                float px = m_screenPos.x + std::cos(angle) * arcRadius;
                float py = m_screenPos.y + std::sin(angle) * arcRadius;
                arc[i].position = {px, py};
                arc[i].color = sf::Color(255, 150, 50, static_cast<uint8_t>(200 * ratio));
            }
            window.draw(arc);
        }
    }

    // Pseudo label above
    sf::Text label(font);
    label.setString(m_pseudo);
    label.setCharacterSize(11);
    label.setFillColor(sf::Color::White);
    
    sf::FloatRect textBounds = label.getLocalBounds();
    label.setOrigin({textBounds.size.x / 2.f, textBounds.size.y});
    label.setPosition({m_screenPos.x, m_screenPos.y - scaledRadius - 6.f});
    window.draw(label);
}

void PlayerAvatar::TriggerPushEffect()
{
    m_pushScale = 1.4f;
    m_flashAlpha = 200.f;
    m_hitFlashColor = sf::Color::White;
    m_shockwaveRadius = AVATAR_RADIUS;
    m_shockwaveAlpha = 255.f;
    m_pushCooldown = PUSH_COOLDOWN_TIME;
}

void PlayerAvatar::TriggerHitEffect()
{
    m_flashAlpha = 220.f;
    m_hitFlashColor = sf::Color(255, 80, 80); // Red flash
    m_pushScale = 1.2f; // Slight scale bump
}

sf::FloatRect PlayerAvatar::GetBounds() const
{
    return sf::FloatRect(
        {m_screenPos.x - AVATAR_RADIUS, m_screenPos.y - AVATAR_RADIUS},
        {AVATAR_RADIUS * 2.f, AVATAR_RADIUS * 2.f}
    );
}
