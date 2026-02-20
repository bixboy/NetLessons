#include "MiniGames/MiniGameRedLightGreenLight.h"
#include "ClientContext.h"
#include "UI/UIRenderer.h"
#include <cmath>
#include <string>


void MiniGameRedLightGreenLight::Draw(ClientContext& m_ctx, UIRenderer& ui)
{
    float winW = static_cast<float>(m_ctx.WindowSize.x);
    float winH = static_cast<float>(m_ctx.WindowSize.y);

    // 1. Draw Lines (Start & Finish)
    float startX = winW * 0.1f;
    float finishX = winW * 0.9f;

    sf::RectangleShape startLine(sf::Vector2f(4.f, winH));
    startLine.setFillColor(sf::Color(255, 255, 255, 100));
    startLine.setPosition({startX, 0.f});
    m_ctx.Target->draw(startLine);

    sf::RectangleShape finishLine(sf::Vector2f(6.f, winH));
    finishLine.setFillColor(sf::Color(50, 255, 50, 150));
    finishLine.setPosition({finishX, 0.f});
    m_ctx.Target->draw(finishLine);

    if (m_ctx.IsIceMode)
    {
        float floorX = startX;
        float floorW = finishX - startX;
        
        sf::RectangleShape iceBase(sf::Vector2f(floorW, winH));
        iceBase.setPosition({floorX, 0.f});
        iceBase.setFillColor(sf::Color(150, 200, 255, 180));
        m_ctx.Target->draw(iceBase);

        static std::vector<sf::Vertex> iceCracks;
        static bool cracksGenerated = false;
        if (!cracksGenerated)
        {
            std::srand(12345);

            for (int i = 0; i < 50; ++i)
            {
                float x1 = floorX + (std::rand() % static_cast<int>(floorW));
                float y1 = std::rand() % static_cast<int>(winH);
                float ang = (std::rand() % 360) * 3.14159f / 180.f;
                float len = 20.f + (std::rand() % 100);
                float x2 = x1 + std::cos(ang) * len;
                float y2 = y1 + std::sin(ang) * len;
                
                uint8_t a = 50 + std::rand() % 100;
                sf::Color col(200, 240, 255, a);
                
                iceCracks.push_back(sf::Vertex({x1, y1}, col));
                iceCracks.push_back(sf::Vertex({x2, y2}, col));
            }
            cracksGenerated = true;
        }
        
        m_ctx.Target->draw(iceCracks.data(), iceCracks.size(), sf::PrimitiveType::Lines);

        float time = m_ctx.AnimClock.getElapsedTime().asSeconds();
        for (int i = 0; i < 20; ++i)
        {
             float offset = i * 0.5f;
             float t = time + offset;
             float brightness = (std::sin(t * 3.f) + 1.f) / 2.f;
             
             if (brightness > 0.8f)
             {
                 float sx = floorX + ((i * 37) % static_cast<int>(floorW));
                 float sy = ((i * 91) % static_cast<int>(winH));
                 
                 sf::CircleShape spark(2.f + brightness * 2.f);
                 spark.setPosition({sx, sy});
                 spark.setFillColor(sf::Color(255, 255, 255, static_cast<uint8_t>(brightness * 255)));
                 m_ctx.Target->draw(spark);
             }
        }
    }

    sf::Text status(m_ctx.Font);
    
    if (m_ctx.TrollLight == 1)
    {
        status.setString("FEU VIOLET !?"); // Violet
        status.setFillColor(sf::Color::Magenta);
    }
    else if (m_ctx.TrollLight == 2)
    {
        status.setString("FEU ORANGE !?"); // Orange
        status.setFillColor(sf::Color(255, 165, 0));
    }
    else if (m_ctx.IsRedLight)
    {
        status.setString("FEU ROUGE !"); // Rouge
        status.setFillColor(sf::Color::Red);
    }
    else
    {
        status.setString("FEU VERT !"); // Vert
        status.setFillColor(sf::Color::Green);
    }
    
    status.setCharacterSize(60);
    status.setStyle(sf::Text::Bold);
    
    // Pulse effect
    float scale = 1.0f + m_ctx.PulseValue * 0.2f;
    status.setScale({scale, scale});

    sf::FloatRect b = status.getLocalBounds();
    status.setOrigin({b.size.x / 2.f, b.size.y / 2.f});
    status.setPosition({winW / 2.f, 100.f});

    m_ctx.Target->draw(status);

    // ICE MODE WARNING
    if (m_ctx.IsIceMode)
    {
        sf::Text iceText(m_ctx.Font);
        iceText.setString("SOL GLISSANT !!!");
        iceText.setCharacterSize(40);
        iceText.setFillColor(sf::Color(100, 200, 255));
        iceText.setStyle(sf::Text::Bold | sf::Text::Italic);
        
        float bob = std::sin(m_ctx.AnimClock.getElapsedTime().asSeconds() * 5.f) * 10.f;
        
        sf::FloatRect ib = iceText.getLocalBounds();
        iceText.setOrigin({ib.size.x / 2.f, ib.size.y / 2.f});
        iceText.setPosition({winW / 2.f, 220.f + bob});
        
        m_ctx.Target->draw(iceText);
    }

    // 3. Helper Text
    sf::Text help(m_ctx.Font);
    help.setString("Avance au vert, arrete-toi au rouge !");
    help.setCharacterSize(20);
    help.setFillColor(sf::Color(200, 200, 200));
    sf::FloatRect hb = help.getLocalBounds();
    help.setOrigin({hb.size.x / 2.f, hb.size.y / 2.f});
    help.setPosition({winW / 2.f, 160.f});
    m_ctx.Target->draw(help);

    // 4. Elimination Feedback
    if (m_ctx.IsSpectator())
    {
        sf::Text elim(m_ctx.Font);
        elim.setString("ELIMINE !");
        elim.setCharacterSize(100);
        elim.setFillColor(sf::Color(255, 0, 0, 200));
        elim.setStyle(sf::Text::Bold);
        elim.setOutlineThickness(4.f);
        elim.setOutlineColor(sf::Color::White);
        
        sf::FloatRect eb = elim.getLocalBounds();
        elim.setOrigin({eb.size.x / 2.f, eb.size.y / 2.f});
        elim.setPosition({winW / 2.f, winH / 2.f});
        
        m_ctx.Target->draw(elim);
    }
}
