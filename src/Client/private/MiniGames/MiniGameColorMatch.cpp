#include "MiniGames/MiniGameColorMatch.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <string>
#include <cstdint>
#include <algorithm>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include "ClientContext.h" 



// Helper for Jagged Lightning
static void DrawLightningBolt(sf::RenderTarget& target, sf::Vector2f start, sf::Vector2f end, sf::Color color, float thickness = 1.f)
{
    sf::Vector2f dir = end - start;
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    if (len < 1.f) 
        return;

    sf::Vector2f norm = dir / len;
    sf::Vector2f perp = {-norm.y, norm.x};

    int segments = 12;
    std::vector<sf::Vector2f> points;
    points.push_back(start);

    for(int i=1; i<segments; ++i)
    {
        float t = static_cast<float>(i) / segments;
        float sway = std::sin(t * 3.14159f); 
        float offset = ((rand() % 100) / 50.f - 1.f) * (len * 0.15f) * sway; 
        
        sf::Vector2f p = start + dir * t + perp * offset;
        points.push_back(p);
    }
    points.push_back(end);

    // Draw Glow
    if (thickness > 1.f)
    {
        sf::VertexArray glow(sf::PrimitiveType::TriangleStrip);
        float glowWidth = thickness * 4.f;
        sf::Color glowColor = color;
        glowColor.a /= 4;

        for(size_t i=0; i<points.size(); ++i)
        {
            sf::Vector2f p = points[i];
            glow.append({p + perp * glowWidth, sf::Color::Transparent});
            glow.append({p - perp * glowWidth, sf::Color::Transparent});
        }
    }

    sf::VertexArray bolt(sf::PrimitiveType::LineStrip);
    for(const auto& p : points)
    {
        bolt.append({p, color});
    }
    
    target.draw(bolt);

    sf::VertexArray glowBolt(sf::PrimitiveType::LineStrip);
    sf::Color glowCol = color; 
    glowCol.a = 50;
    
    for(const auto& p : points)
    {
        glowBolt.append({p + sf::Vector2f(2.f, 2.f), glowCol});
    }
    
    target.draw(glowBolt);
    glowBolt.clear();
    
    for(const auto& p : points)
    {
        glowBolt.append({p + sf::Vector2f(-2.f, -2.f), glowCol});
    }
    
    target.draw(glowBolt);
}

MiniGameColorMatch::MiniGameColorMatch(ClientContext& ctx): m_ctx(ctx)
{
    Init();
}

void MiniGameColorMatch::Init()
{
    m_targetColor = -1;
    m_stateTimer = 0.f;
    m_visualTimer = 0.f;
    m_phase = EPhase::Warmup;
    m_gridColors.assign(16, 0);
    
    std::cout << "[CLIENT] Init Color Match" << std::endl;
}

sf::Color MiniGameColorMatch::GetColorFromID(int id)
{
    switch(id)
    {
        case 0: return sf::Color::Red;
        case 1: return sf::Color::Green;
        case 2: return sf::Color::Blue;
        case 3: return sf::Color::Yellow;
        default: return sf::Color::White;
    }
}

void MiniGameColorMatch::OnPacket(const PacketGameData& pkt)
{
    if (pkt.Value == static_cast<int>(EGameDataType::ColorGrid))
    {
        std::stringstream ss(pkt.ExtraData);
        std::string segment;
        m_gridColors.clear();
        
        while(std::getline(ss, segment, ','))
        {
            m_gridColors.push_back(std::stoi(segment));
        }
    }
    else if (pkt.Value == static_cast<int>(EGameDataType::ColorTension))
    {
        // ALL RED
        m_phase = EPhase::Tension;
        m_stateTimer = 5.0f;
        m_visualTimer = 0.f;
        m_targetColor = -1; 
    }
    else if (pkt.Value == static_cast<int>(EGameDataType::ColorReveal))
    {
        // REVEAL COLORS
        m_phase = EPhase::Reveal;
        m_stateTimer = pkt.Timer;
        m_targetColor = std::stoi(pkt.ExtraData);
        m_visualTimer = 0.f;
    }
    else if (pkt.Value == static_cast<int>(EGameDataType::ColorElimination))
    {
        // Trigger Flash
        m_phase = EPhase::Elimination;
        m_stateTimer = 2.0f;
        m_shakeTimer = 0.5f;
        m_shakeMagnitude = 15.f;
        
        sf::Vector2u winSize = m_ctx.Window.getSize();
        for(int i=0; i<50; ++i) {
            float x = static_cast<float>(rand() % winSize.x);
            float y = static_cast<float>(rand() % winSize.y);
            SpawnParticles({x, y}, sf::Color::White, 1);
        }
    }
}


void MiniGameColorMatch::Update(float dt)
{
    if (m_stateTimer > 0.f)
        m_stateTimer -= dt;

    m_visualTimer += dt;
        
    if (m_shakeTimer > 0.f)
        m_shakeTimer -= dt;
        
    for (auto it = m_particles.begin(); it != m_particles.end();)
    {
        it->life -= dt;
        if (it->life <= 0.f)
        {
            it = m_particles.erase(it);
        }
        else
        {
            it->pos += it->vel * dt;
            it++;
        }
    }
}

void MiniGameColorMatch::SpawnParticles(sf::Vector2f pos, sf::Color color, int count)
{
    for(int i=0; i<count; ++i)
    {
        float angle = (rand() % 360) * 3.14159f / 180.f;
        float speed = 50.f + (rand() % 150);
        
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.color = color;
        p.life = 0.5f + ((rand() % 10) / 10.f);
        
        m_particles.push_back(p);
    }
}

void MiniGameColorMatch::Draw(ClientContext& ctx, UIRenderer& ui)
{
    sf::View originalView = m_ctx.Target->getView();
    
    if (m_shakeTimer > 0.f)
    {
        float offsetX = ((rand() % 100) / 50.f - 1.f) * m_shakeMagnitude;
        float offsetY = ((rand() % 100) / 50.f - 1.f) * m_shakeMagnitude;
        
        sf::View shakeView = originalView;
        shakeView.move({offsetX, offsetY});
        m_ctx.Target->setView(shakeView);
    }

    sf::Vector2u winSize = m_ctx.Window.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);
    
    float cellW = winW / GRID_W;
    float cellH = winH / GRID_H;
    
    for (int y = 0; y < GRID_H; ++y)
    {
        for (int x = 0; x < GRID_W; ++x)
        {
            int idx = y * GRID_W + x;
            if (idx >= m_gridColors.size()) 
                continue;
            
            sf::Color baseCol = sf::Color::Black;
            sf::Color glowCol = sf::Color::Transparent;

            // --- STATE MACHINE VISUALS ---
            if (m_phase == EPhase::Warmup || m_phase == EPhase::Reset)
            {
                if ((rand() % 100) > 95)
                {
                    baseCol = sf::Color(30,30,30);
                }
                else
                {
                    baseCol = sf::Color(5,5,5);
                }
            }
            else if (m_phase == EPhase::Tension)
            {
                // NEON RED
                float pulse = (std::sin(m_visualTimer * 3.f) + 1.f) * 0.5f;
                uint8_t r = static_cast<uint8_t>(100 + pulse * 155);
                baseCol = sf::Color(r, 0, 0);
                glowCol = sf::Color(255, 0, 0, 50);
            }
            else if (m_phase == EPhase::Reveal || m_phase == EPhase::Elimination)
            {
                baseCol = GetColorFromID(m_gridColors[idx]);
                
                if (m_phase == EPhase::Elimination)
                {
                    bool isWrong = (m_gridColors[idx] != m_targetColor);
                    if (isWrong)
                    {
                        if ((rand() % 10) > 5)
                        {
                            baseCol = sf::Color::White; // survive
                        }
                        else
                        {
                            baseCol = sf::Color(50, 50, 50); // burn
                        }
                    }
                }
                else // Reveal
                {
                    float time = m_stateTimer * 2.f; 
                    float pulse = (std::sin(time + idx) + 1.f) / 4.f;
                    baseCol.r = static_cast<std::uint8_t>((std::min)(255, baseCol.r + static_cast<int>(pulse * 50)));
                    baseCol.g = static_cast<std::uint8_t>((std::min)(255, baseCol.g + static_cast<int>(pulse * 50)));
                    baseCol.b = static_cast<std::uint8_t>((std::min)(255, baseCol.b + static_cast<int>(pulse * 50)));
                }
            }

            // Draw Tile
            sf::RectangleShape cell({cellW, cellH});
            cell.setPosition({x * cellW, y * cellH});
            cell.setFillColor(sf::Color(10, 10, 15));
            cell.setOutlineColor(sf::Color::Black);
            cell.setOutlineThickness(2.f);
            m_ctx.Target->draw(cell);

            float padding = 4.f;
            sf::RectangleShape innerCell({cellW - padding*2, cellH - padding*2});
            innerCell.setPosition({x * cellW + padding, y * cellH + padding});
            innerCell.setFillColor(baseCol);

            // VFX ELIMINATION
            if (m_phase == EPhase::Elimination && m_gridColors[idx] != m_targetColor)
            {
                // 1. Dark burnt
                innerCell.setFillColor(sf::Color(20, 20, 25)); 
                m_ctx.Target->draw(innerCell);

                // 2. Strobe Overlay
                if ((rand() % 10) > 6)
                {
                    sf::RectangleShape flash = innerCell;
                    flash.setFillColor(sf::Color(200, 230, 255, 100));
                    m_ctx.Target->draw(flash);
                }

                // 3. Lightning Bolts
                int numBolts = 2 + (rand() % 3);
                sf::Vector2f center = {x * cellW + cellW/2.f, y * cellH + cellH/2.f};
                
                for(int b=0; b<numBolts; ++b)
                {
                   float angle = (rand() % 360) * 3.14159f / 180.f;
                   float dist = cellW * 0.7f;
                   sf::Vector2f edge = {center.x + std::cos(angle)*dist, center.y + std::sin(angle)*dist};
                   
                   DrawLightningBolt(*m_ctx.Target, center, edge, sf::Color(200, 255, 255), 2.f);
                   
                   if (rand() % 2 == 0) {
                       float branchAngle = angle + ((rand()%100)/50.f - 1.f) * 0.5f;
                       sf::Vector2f branchEdge = {center.x + std::cos(branchAngle)*dist*0.6f, center.y + std::sin(branchAngle)*dist*0.6f};
                       DrawLightningBolt(*m_ctx.Target, center, branchEdge, sf::Color(100, 200, 255), 1.f);
                   }
                }
            }
            else
            {
                 m_ctx.Target->draw(innerCell);
            }

            if (m_phase == EPhase::Tension)
            {
                innerCell.setOutlineColor(glowCol);
                innerCell.setOutlineThickness(5.f);
                
                innerCell.setFillColor(sf::Color::Transparent);
                m_ctx.Target->draw(innerCell);
            } 
            else if (m_phase != EPhase::Elimination)
            { 
               innerCell.setOutlineColor(sf::Color(255,255,255,30));
               innerCell.setOutlineThickness(-2.f);
               innerCell.setFillColor(sf::Color::Transparent);
               m_ctx.Target->draw(innerCell);
            }
        }
    }
    
    // --- HUD ---
    if (m_phase == EPhase::Reveal && m_targetColor != -1)
    {
        sf::Color targetCol = GetColorFromID(m_targetColor);
        sf::RectangleShape banner({winW, 80.f});
        
        banner.setPosition({0.f, 20.f});
        banner.setFillColor(sf::Color(0,0,0,150));
        m_ctx.Target->draw(banner);
        
        sf::RectangleShape box({50.f, 50.f});
        
        box.setPosition({winW/2.f - 25.f, 35.f});
        box.setFillColor(targetCol);
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(2.f);
        m_ctx.Target->draw(box);
        
        // Timer
         sf::Text timerTxt(m_ctx.Font, std::to_string((int)std::ceil(m_stateTimer)), 60);
         timerTxt.setPosition({winW/2.f + 50.f, 25.f});
         m_ctx.Target->draw(timerTxt);
    }
    
    // --- PARTICLES ---
    for (const auto& p : m_particles)
    {
        sf::RectangleShape ps({6.f, 6.f});
        ps.setPosition(p.pos);
        
        sf::Color pc = p.color;
        pc.a = static_cast<std::uint8_t>(255 * (p.life / 0.5f));
        ps.setFillColor(pc);
        
        m_ctx.Target->draw(ps);
    }

    // Restore View
    if (m_shakeTimer > 0.f)
    {
        m_ctx.Target->setView(originalView);
    }
}
