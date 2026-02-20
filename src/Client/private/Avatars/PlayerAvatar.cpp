#include "Avatars/PlayerAvatar.h"
#include <cmath>
#include <algorithm>
#include <random>


PlayerAvatar::PlayerAvatar(const std::string& pseudo, sf::Color color, bool isLocal) : m_pseudo(pseudo), m_color(color), m_isLocal(isLocal)
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
    if (m_state == EPlayerState::Dead) canInput = false;

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

        constexpr float SVR_ICE_ACCEL = 1.2f;
        constexpr float SVR_ICE_DRAG = 2.0f;
        constexpr float SVR_MAX_SPEED = 0.5f;

        if (m_isIceMode)
        {
            float accelX = dirX * SVR_ICE_ACCEL * m_areaW;
            float accelY = dirY * SVR_ICE_ACCEL * m_areaH;

            m_velocity.x += accelX * dt;
            m_velocity.y += accelY * dt;

            float dragFactor = 1.0f - (SVR_ICE_DRAG * dt);
            if (dragFactor < 0.f)
                dragFactor = 0.f;
            
            m_velocity.x *= dragFactor;
            m_velocity.y *= dragFactor;
            
            float vxNorm = m_velocity.x / m_areaW;
            float vyNorm = m_velocity.y / m_areaH;
            float spdSq = vxNorm*vxNorm + vyNorm*vyNorm;
            if (spdSq > SVR_MAX_SPEED*SVR_MAX_SPEED)
            {
                float scale = SVR_MAX_SPEED / std::sqrt(spdSq);
                m_velocity.x *= scale;
                m_velocity.y *= scale;
            }
        }
        else
        {
            // --- PHYSICS ---
            
            float maxSpeed = NORM_SPEED * m_areaW;
            float targetVX = dirX * maxSpeed;
            float targetVY = dirY * NORM_SPEED * m_areaH;
            
            float accel = ACCEL; 
            m_velocity.x += (targetVX - m_velocity.x) * accel * dt;
            m_velocity.y += (targetVY - m_velocity.y) * accel * dt;
        }

        m_screenPos.x += m_velocity.x * dt;
        m_screenPos.y += m_velocity.y * dt;

        m_screenPos.x = std::clamp(m_screenPos.x, m_areaX, m_areaX + m_areaW);
        m_screenPos.y = std::clamp(m_screenPos.y, m_areaY, m_areaY + m_areaH);

        sf::Vector2f targetScreen = NormalizedToScreen(
            std::clamp(m_targetNorm.x, 0.f, 1.f),
            std::clamp(m_targetNorm.y, 0.f, 1.f)
        );
        
        float diffX = targetScreen.x - m_screenPos.x;
        float diffY = targetScreen.y - m_screenPos.y;
        float dist = std::sqrt(diffX * diffX + diffY * diffY);

        float thresh = m_isIceMode ? 50.f : 5.f; 
        
        if (dist > thresh)
        {
            float strength = m_isIceMode ? 2.0f : CORRECTION_STRENGTH;
            
            float t = std::min(1.f, strength * dt * (dist / 100.f));
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

    if (m_pushScale > 1.f)
        m_pushScale = std::max(1.f, m_pushScale - 2.f * dt);

    if (m_flashAlpha > 0.f)
        m_flashAlpha = std::max(0.f, m_flashAlpha - 600.f * dt);

    if (m_shockwaveAlpha > 0.f)
    {
        m_shockwaveRadius += 200.f * dt;
        m_shockwaveAlpha = std::max(0.f, m_shockwaveAlpha - 400.f * dt);
    }
    
    if (m_explosionAlpha > 0.f)
    {
        m_explosionRadius += 600.f * dt;
        m_explosionAlpha = std::max(0.f, m_explosionAlpha - 300.f * dt);
    }

    if (m_pushCooldown > 0.f)
        m_pushCooldown = std::max(0.f, m_pushCooldown - dt);

    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        it->life -= dt;
        if (it->life <= 0.f)
        {
            it = m_particles.erase(it);
        }
        else
        {
            it->vel *= (1.f - it->drag * dt);
            it->pos += it->vel * dt;
            ++it;
        }
    }
}

void PlayerAvatar::Draw(sf::RenderTarget& target, sf::Font& font)
{
    if (m_state == EPlayerState::Spectating)
        return;

    float pulse = 0.f;
    if (m_isLocal)
    {
        pulse = std::sin(static_cast<float>(clock()) / 200.f) * 2.f;
    }

    if (m_shockwaveAlpha > 0.f)
    {
        sf::CircleShape shockwave(m_shockwaveRadius);
        shockwave.setOrigin({m_shockwaveRadius, m_shockwaveRadius});
        shockwave.setPosition(m_screenPos);
        shockwave.setFillColor(sf::Color::Transparent);
        shockwave.setOutlineThickness(5.f);
        shockwave.setOutlineColor(sf::Color(255, 255, 255, static_cast<uint8_t>(m_shockwaveAlpha)));
        target.draw(shockwave);
    }
    
    float scaledRadius = AVATAR_RADIUS * m_pushScale + pulse;

    sf::CircleShape shape(scaledRadius);
    shape.setOrigin({scaledRadius, scaledRadius});
    shape.setPosition(m_screenPos);
    
    sf::Color drawColor = m_color;
    if (m_isIceMode) 
        drawColor = sf::Color(100, 200, 255);
    
    // Hit Flash
    if (m_flashAlpha > 0.f)
    {
        float ratio = m_flashAlpha / 255.f;
        drawColor.r = static_cast<uint8_t>(std::min(255.f, drawColor.r + (m_hitFlashColor.r - drawColor.r) * ratio));
        drawColor.g = static_cast<uint8_t>(std::min(255.f, drawColor.g + (m_hitFlashColor.g - drawColor.g) * ratio));
        drawColor.b = static_cast<uint8_t>(std::min(255.f, drawColor.b + (m_hitFlashColor.b - drawColor.b) * ratio));
    }
    
    shape.setFillColor(drawColor);
    shape.setOutlineThickness(m_isLocal ? 3.f : 2.f);
    shape.setOutlineColor(m_isLocal ? sf::Color::Yellow : sf::Color::Black);

    target.draw(shape);

    // BOMB Visual
    if (m_isBombHolder)
    {
        sf::CircleShape bombGlow(scaledRadius + 5.f + pulse * 2.f);
        bombGlow.setOrigin({scaledRadius + 5.f + pulse * 2.f, scaledRadius + 5.f + pulse * 2.f});
        bombGlow.setPosition(m_screenPos);
        bombGlow.setFillColor(sf::Color::Transparent);
        bombGlow.setOutlineThickness(4.f);
        bombGlow.setOutlineColor(sf::Color::Red);
        target.draw(bombGlow);
    }

    // --- HUD ---
    if (m_pushCooldown > 0.f)
    {
        float ratio = m_pushCooldown / PUSH_COOLDOWN_TIME;
        
        // Background
        sf::RectangleShape barBg({40.f, 6.f});
        barBg.setOrigin({20.f, 3.f});
        barBg.setPosition({m_screenPos.x, m_screenPos.y - scaledRadius - 15.f});
        barBg.setFillColor(sf::Color(50, 50, 50));
        target.draw(barBg);
        
        // Fill
        sf::RectangleShape bar({40.f * ratio, 6.f});
        bar.setOrigin({20.f, 3.f});
        bar.setPosition({m_screenPos.x, m_screenPos.y - scaledRadius - 15.f});
        bar.setFillColor(sf::Color(100, 255, 100));
        target.draw(bar);
    }
    
    if (m_shockwaveAlpha > 100.f && m_isLocal)
    {
        // Screen shake
    }
    
    // --- Push Range ---
    if (m_isLocal)
    {
        float baseAngle = 0.f;
        if (std::abs(m_velocity.x) > 0.0001f || std::abs(m_velocity.y) > 0.0001f)
            baseAngle = std::atan2(m_velocity.y, m_velocity.x);

        float arcRadius = 40.f;
        float startAngle = -0.5f;
        float endAngle = 0.5f;
        sf::VertexArray arc(sf::PrimitiveType::LineStrip, 11);

        for (int i = 0; i <= 10; ++i)
        {
            float ratio = static_cast<float>(i) / 10.f;
            float angle = baseAngle + startAngle + (endAngle - startAngle) * ratio;
            
            float px = m_screenPos.x + std::cos(angle) * arcRadius;
            float py = m_screenPos.y + std::sin(angle) * arcRadius;
            
            arc[i].position = {px, py};
            arc[i].color = sf::Color(255, 150, 50, static_cast<uint8_t>(200 * (1.f - std::abs(0.5f - ratio) * 2.f))); 
        }
        target.draw(arc);
    }

    // Pseudo label above
    sf::Text label(font);
    label.setString(m_pseudo);
    label.setCharacterSize(11);
    label.setFillColor(sf::Color::White);
    
    sf::FloatRect textBounds = label.getLocalBounds();
    label.setOrigin({textBounds.size.x / 2.f, textBounds.size.y});
    label.setPosition({m_screenPos.x, m_screenPos.y - scaledRadius - 6.f});
    target.draw(label);

    // EXPLOSION EFFECT (Draw last to overlay everything)
    if (m_explosionAlpha > 0.f)
    {
        // Core (White/Yellow)
        sf::CircleShape core(m_explosionRadius);
        core.setOrigin({m_explosionRadius, m_explosionRadius});
        core.setPosition(m_screenPos);
        core.setFillColor(sf::Color(255, 255, 200, static_cast<uint8_t>(m_explosionAlpha)));
        target.draw(core);

        // Outer Ring (Red/Orange)
        sf::CircleShape ring(m_explosionRadius * 0.9f);
        ring.setOrigin({m_explosionRadius * 0.9f, m_explosionRadius * 0.9f});
        ring.setPosition(m_screenPos);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(10.f);
        ring.setOutlineColor(sf::Color(255, 100, 0, static_cast<uint8_t>(m_explosionAlpha * 0.8f))); 
        target.draw(ring);
    }
    
    // Draw Particles
    for (const auto& p : m_particles)
    {
        sf::RectangleShape part({p.size, p.size});
        part.setPosition(p.pos);
        part.setOrigin({p.size / 2.f, p.size / 2.f}); // SFML 3 fix in mind
        
        uint8_t alpha = static_cast<uint8_t>(255.f * (p.life / p.maxLife));
        sf::Color c = p.color;
        c.a = alpha;
        part.setFillColor(c);
        
        target.draw(part);
    }
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

void PlayerAvatar::TriggerExplosionEffect()
{
    m_explosionAlpha = 255.f;
    m_explosionRadius = AVATAR_RADIUS * 0.5f;
    m_flashAlpha = 255.f;
    m_hitFlashColor = sf::Color::White;
    
    SpawnGore();
}

void PlayerAvatar::SpawnGore()
{
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution angleDist(0.f, 360.f);
    std::uniform_real_distribution speedDist1(200.f, 500.f);
    std::uniform_real_distribution lifeDist1(0.5f, 1.5f);
    std::uniform_real_distribution sizeDist1(2.f, 4.f);
    
    std::uniform_real_distribution speedDist2(50.f, 150.f);
    std::uniform_real_distribution lifeDist2(2.0f, 4.0f);
    std::uniform_real_distribution sizeDist2(5.f, 10.f);

    // 1. Blood Spray
    for (int i = 0; i < 80; ++i)
    {
        float angle = angleDist(rng) * 3.14159f / 180.f;
        float speed = speedDist1(rng);
        
        Particle p;
        p.pos = m_screenPos;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.maxLife = lifeDist1(rng);
        p.life = p.maxLife;
        p.size = sizeDist1(rng);
        p.color = sf::Color(200, 0, 0);
        p.drag = 3.f;
        
        m_particles.push_back(p);
    }
    
    // 2. Meat
    for (int i = 0; i < 15; ++i)
    {
        float angle = angleDist(rng) * 3.14159f / 180.f;
        float speed = speedDist2(rng);
        
        Particle p;
        p.pos = m_screenPos;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.maxLife = lifeDist2(rng);
        p.life = p.maxLife;
        p.size = sizeDist2(rng);
        p.color = sf::Color(100, 0, 0);
        p.drag = 1.0f;
        
        m_particles.push_back(p);
    }
}

sf::FloatRect PlayerAvatar::GetBounds() const
{
    return sf::FloatRect({
        m_screenPos.x - AVATAR_RADIUS, 
        m_screenPos.y - AVATAR_RADIUS},
        {
            AVATAR_RADIUS * 2.f,
            AVATAR_RADIUS * 2.f
        }
    );
}
