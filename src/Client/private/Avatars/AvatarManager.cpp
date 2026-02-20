#include "Avatars/AvatarManager.h"
#include "ClientContext.h"
#include <cmath>
#include <cstdlib>


AvatarManager::AvatarManager(ClientContext& ctx) : m_ctx(ctx)
{
}

void AvatarManager::InitLocalAvatar(const std::string& pseudo)
{
    m_localAvatar = PlayerAvatar(pseudo, sf::Color::White, true);
    m_localAvatar.SetPlayArea(20.f, 20.f,
        static_cast<float>(m_ctx.WindowSize.x) - 40.f,
        static_cast<float>(m_ctx.WindowSize.y) - 120.f);
}

void AvatarManager::AddRemoteAvatar(const std::string& pseudo, sf::Color color)
{
    m_remoteAvatars.emplace(pseudo, PlayerAvatar(pseudo, color, false));
}

void AvatarManager::RemoveRemoteAvatar(const std::string& pseudo)
{
    m_remoteAvatars.erase(pseudo);
}

void AvatarManager::SetAvatarTarget(const std::string& pseudo, float x, float y)
{
    auto it = m_remoteAvatars.find(pseudo);
    if (it != m_remoteAvatars.end())
    {
        it->second.SetTargetNormalized(x, y);
    }
}

void AvatarManager::SetLocalTarget(float x, float y)
{
    m_localAvatar.SetTargetNormalized(x, y);
}

void AvatarManager::SetSpectator(const std::string& pseudo, bool isSpectator)
{
    if (m_localAvatar.GetPseudo() == pseudo)
    {
        m_localAvatar.SetSpectator(isSpectator);
    }
    else
    {
        auto it = m_remoteAvatars.find(pseudo);
        if (it != m_remoteAvatars.end())
        {
            it->second.SetSpectator(isSpectator);
        }
    }
}

void AvatarManager::SetAvatarState(const std::string& pseudo, EPlayerState state)
{
    auto it = m_remoteAvatars.find(pseudo);
    if (it != m_remoteAvatars.end())
    {
        it->second.SetState(state);
    }
}

void AvatarManager::UpdateVisibility(EPlayerState localState)
{
    m_localAvatar.SetState(localState);
    m_localAvatar.SetSpectator(localState == EPlayerState::Spectating);

    for (auto& [name, avatar] : m_remoteAvatars)
    {
        EPlayerState remoteState = avatar.GetState();
        bool shouldHide = false;

        if (remoteState == EPlayerState::Spectating)
        {
            shouldHide = true;
        }
        else if (m_ctx.State == ClientState::Lobby || m_ctx.State == ClientState::Result)
        {
            shouldHide = (remoteState == EPlayerState::Playing);
        }
        else if (m_ctx.State == ClientState::Game)
        {
            shouldHide = (remoteState == EPlayerState::Lobby);
        }
        
        avatar.SetSpectator(shouldHide);
    }
}

void AvatarManager::SendInput()
{
    if (m_localAvatar.GetState() == EPlayerState::Dead) 
        return;

    if (m_inputSendClock.getElapsedTime().asSeconds() < 0.033f)
        return;
    m_inputSendClock.restart();

    int8_t dirX = 0, dirY = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        dirY = -1;
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        dirY = 1;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        dirX = -1;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        dirX = 1;

    PacketPlayerInput pkt;
    pkt.DirX = dirX;
    pkt.DirY = dirY;
    pkt.Push = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    m_ctx.Network.Send(pkt);
}

void AvatarManager::Update(float dt, bool canInput)
{
    float areaW = static_cast<float>(m_ctx.WindowSize.x) - 40.f;
    float areaH = static_cast<float>(m_ctx.WindowSize.y) - 120.f;
    m_localAvatar.SetPlayArea(20.f, 20.f, areaW, areaH);

    UpdateVisibility(m_localAvatar.GetState());

    m_localAvatar.Update(dt, canInput);
    for (auto& [name, avatar] : m_remoteAvatars)
    {
        avatar.SetPlayArea(20.f, 20.f, areaW, areaH);
        avatar.Update(dt);
    }

    if (m_shakeTimer > 0.f)
    {
        m_shakeTimer -= dt;
        m_shakeIntensity *= 0.9f;
    }
    else
    {
        m_shakeIntensity = 0.f;
    }

    for (auto it = m_groundBlood.begin(); it != m_groundBlood.end();)
    {
        float decay = it->decaySpeed * dt;
        it->alpha -= decay;

        if (it->alpha <= 0.f)
        {
            it = m_groundBlood.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void AvatarManager::Draw(sf::Font& font)
{
    for (const auto& zone : m_interactionZones)
    {
        sf::FloatRect avatarBounds = m_localAvatar.GetBounds();
        bool isNear = zone.bounds.findIntersection(avatarBounds).has_value();

        if (isNear)
        {
            sf::RectangleShape highlight({zone.bounds.size.x, zone.bounds.size.y});
            highlight.setPosition({zone.bounds.position.x, zone.bounds.position.y});
            highlight.setFillColor(sf::Color(255, 255, 100, 30));
            highlight.setOutlineThickness(2.f);
            highlight.setOutlineColor(sf::Color(255, 255, 100, static_cast<uint8_t>(100 + m_ctx.PulseValue * 155)));
            m_ctx.Target->draw(highlight);

            sf::Text prompt(font);
            prompt.setString("[E]");
            prompt.setCharacterSize(14);
            prompt.setFillColor(sf::Color(255, 255, 100));
            prompt.setStyle(sf::Text::Bold);
            sf::FloatRect promptBounds = prompt.getLocalBounds();

            prompt.setPosition({
                zone.bounds.position.x + zone.bounds.size.x / 2.f - promptBounds.size.x / 2.f,
                zone.bounds.position.y - 20.f
            });

            m_ctx.Target->draw(prompt);
        }
    }

    sf::View originalView = m_ctx.Target->getView();
    if (m_shakeIntensity > 0.5f)
    {
        sf::View shakenView = originalView;
        float ox = (static_cast<float>(std::rand() % 100) / 50.f - 1.f) * m_shakeIntensity;
        float oy = (static_cast<float>(std::rand() % 100) / 50.f - 1.f) * m_shakeIntensity;
        shakenView.move({ox, oy});
        m_ctx.Target->setView(shakenView);
    }

    if (m_ctx.ActiveGameID == 1)
        m_localAvatar.SetBombHolder(m_localAvatar.GetPseudo() == m_ctx.BombHolder);
    else
        m_localAvatar.SetBombHolder(false);

    // Draw Ground Blood (Under Everything)
    for (const auto& blood : m_groundBlood)
    {
        sf::CircleShape stain(20.f);
        stain.setScale({blood.scale, blood.scale * 0.8f});
        stain.setOrigin({20.f, 20.f});
        stain.setPosition(blood.pos);
        stain.setRotation(sf::degrees(blood.rotation));
        
        sf::Color c = blood.color;
        c.a = static_cast<uint8_t>(blood.alpha);
        stain.setFillColor(c);
        
        m_ctx.Target->draw(stain);
    }

    for (auto& [name, avatar] : m_remoteAvatars)
    {
        if (m_ctx.ActiveGameID == 1)
            avatar.SetBombHolder(name == m_ctx.BombHolder);
        else
            avatar.SetBombHolder(false);

        avatar.Draw(*m_ctx.Target, font);
    }

    if (m_ctx.ActiveGameID == 1)
        m_localAvatar.SetBombHolder(m_localAvatar.GetPseudo() == m_ctx.BombHolder);
    else
        m_localAvatar.SetBombHolder(false);

    m_localAvatar.Draw(*m_ctx.Target, font);
    m_ctx.Target->setView(originalView);
}

void AvatarManager::BuildInteractionZones()
{
    m_interactionZones.clear();

    float btnWidth = 320.f;
    float btnHeight = 45.f;
    float btnX = m_ctx.CenterX - btnWidth / 2.f;

    if (m_ctx.State == ClientState::Lobby)
    {
        float startY = 220.f;

        if (m_ctx.IsGameRunning)
        {
            float btnGap = 50.f;
            float y = startY + btnGap;
             m_interactionZones.push_back({
                sf::FloatRect({btnX, y - btnHeight / 2.f}, {btnWidth, btnHeight}),
                "OBSERVER",
                [this]() {
                    PacketGameStart pkt;
                    pkt.GameID = m_ctx.ActiveGameID;
                    m_ctx.Network.Send(pkt);
                }
            });
        }
        else
        {
            float btnGap = 60.f;
            for (size_t i = 0; i < m_ctx.AvailableGames.size(); ++i)
            {
                const auto& game = m_ctx.AvailableGames[i];
                float y = startY + i * btnGap;

                m_interactionZones.push_back({
                    sf::FloatRect({btnX, y - btnHeight / 2.f}, {btnWidth, btnHeight}),
                    game.Name,
                    [this, game]() {
                        PacketGameStart pkt;
                        pkt.GameID = game.ID;
                        m_ctx.Network.Send(pkt);
                    }
                });
            }
        }
    }
    else if (m_ctx.State == ClientState::Result)
    {
        float centerY = static_cast<float>(m_ctx.WindowSize.y) / 2.f;
        float offset = centerY - 225.f;
        float y = 350.f + offset;
        m_interactionZones.push_back({
            sf::FloatRect({btnX, y - btnHeight / 2.f}, {btnWidth, btnHeight}),
            "REJOUER",
            [this]() {
                PacketGameStart pkt;
                m_ctx.Network.Send(pkt);
                m_ctx.ServerMessage = "En attente du serveur...";
            },
            3.f, false
        });
    }
    else if (m_ctx.State == ClientState::Game)
    {
        // Juste Prix Interaction Zones
        if (m_ctx.ActiveGameID == 0 && !m_ctx.IsSpectator())
        {
            float winW = static_cast<float>(m_ctx.WindowSize.x);
            float centerY = static_cast<float>(m_ctx.WindowSize.y) / 2.f;
            float offset = centerY - 230.f;

            float panelW = 120.f;
            float panelH = 120.f;
            float panelY = 180.f + offset;

            // "+" button — LEFT side
            float leftX = 40.f;
            m_interactionZones.push_back({
                sf::FloatRect({leftX, panelY}, {panelW, panelH}),
                "+",
                [this]() {
                    if (m_ctx.CurrentNumberChoice < 100)
                    {
                        m_ctx.CurrentNumberChoice++;
                        m_ctx.Sound.Play(SoundType::Select);
                    }
                },
                0.f, false
            });

            // "-" button — RIGHT side
            float rightX = winW - 40.f - panelW;
            m_interactionZones.push_back({
                sf::FloatRect({rightX, panelY}, {panelW, panelH}),
                "-",
                [this]() {
                    if (m_ctx.CurrentNumberChoice > 0)
                    {
                        m_ctx.CurrentNumberChoice--;
                        m_ctx.Sound.Play(SoundType::Select);
                    }
                },
                0.f, false
            });

            // "VALIDER" button — CENTER BOTTOM
            float validerY = 400.f + offset - btnHeight / 2.f;
            m_interactionZones.push_back({
                sf::FloatRect({btnX, validerY}, {btnWidth, btnHeight}),
                "VALIDER",
                [this]() {
                    PacketGameData pkt;
                    pkt.Value = m_ctx.CurrentNumberChoice;
                    m_ctx.Network.Send(pkt);
                    m_ctx.ServerMessage = "Envoyé : " + std::to_string(m_ctx.CurrentNumberChoice);
                    m_ctx.MessageColor = sf::Color(255, 200, 100);
                    m_ctx.Sound.Play(SoundType::Select);
                },
                0.f, false
            });
        }
    }
}

void AvatarManager::CheckInteraction(float dt, const ChatBox& chat)
{
    sf::Vector2f avatarPos = m_localAvatar.GetPosition();
    bool intersecting = false;
    bool ePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E) && !chat.IsTyping() && m_ctx.Window.hasFocus();

    for (const auto& zone : m_interactionZones)
    {
        bool inside = avatarPos.x >= zone.bounds.position.x &&
                      avatarPos.x < zone.bounds.position.x + zone.bounds.size.x &&
                      avatarPos.y >= zone.bounds.position.y &&
                      avatarPos.y < zone.bounds.position.y + zone.bounds.size.y;

        if (inside)
        {
            intersecting = true;

            if (ePressed)
            {
                if (zone.holdTime <= 0.f)
                {
                    // Instant: trigger once on E press (edge detection)
                    if (!m_wasEPressed)
                        zone.action();
                }
                else
                {
                    m_interactionTimer += dt;
                    if (m_interactionTimer >= zone.holdTime)
                    {
                        zone.action();
                        m_interactionTimer = 0.f;
                    }
                }
            }
            else
            {
                m_interactionTimer = 0.f;
            }
            break;
        }
    }

    if (!intersecting)
        m_interactionTimer = 0.f;

    m_wasEPressed = ePressed;
}

void AvatarManager::OnPushAction(const std::string& pusherPseudo)
{
    // Trigger push effect on the pusher
    bool isLocal = (pusherPseudo == m_localAvatar.GetPseudo());

    if (isLocal)
    {
        m_localAvatar.TriggerPushEffect();
    }
    else
    {
        auto it = m_remoteAvatars.find(pusherPseudo);
        if (it != m_remoteAvatars.end())
            it->second.TriggerPushEffect();
    }

    // Determine push origin position
    sf::Vector2f pushOrigin;
    if (isLocal)
        pushOrigin = m_localAvatar.GetPosition();
    else
    {
        auto it = m_remoteAvatars.find(pusherPseudo);
        if (it != m_remoteAvatars.end())
            pushOrigin = it->second.GetPosition();
        else
            return;
    }

    // Hit effect on nearby avatars (within ~80px screen-space)
    constexpr float HIT_RADIUS_SQ = 80.f * 80.f;

    if (!isLocal)
    {
        // Check local avatar for hit
        sf::Vector2f diff = m_localAvatar.GetPosition() - pushOrigin;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        if (distSq < HIT_RADIUS_SQ)
        {
            m_localAvatar.TriggerHitEffect();
            m_shakeIntensity = 6.f;
            m_shakeTimer = 0.3f;
        }
    }

    for (auto& [name, avatar] : m_remoteAvatars)
    {
        if (name == pusherPseudo) continue;

        sf::Vector2f diff = avatar.GetPosition() - pushOrigin;
        float distSq = diff.x * diff.x + diff.y * diff.y;
        if (distSq < HIT_RADIUS_SQ)
        {
            avatar.TriggerHitEffect();
        }
    }

}

void AvatarManager::TriggerExplosion(const std::string& pseudo)
{
    if (m_localAvatar.GetPseudo() == pseudo)
    {
        m_localAvatar.TriggerExplosionEffect();
        SpawnGroundBlood(m_localAvatar.GetPosition().x, m_localAvatar.GetPosition().y);
        
        // Screen Shake
        m_shakeIntensity = 15.f;
        m_shakeTimer = 0.5f;
    }
    else
    {
        auto it = m_remoteAvatars.find(pseudo);
        if (it != m_remoteAvatars.end())
        {
            it->second.TriggerExplosionEffect();
            SpawnGroundBlood(it->second.GetPosition().x, it->second.GetPosition().y);
        }
    }
}

void AvatarManager::SetIceMode(bool enabled)
{
    m_localAvatar.SetIceMode(enabled);
    for (auto& [pseudo, avatar] : m_remoteAvatars)
    {
        avatar.SetIceMode(enabled);
    }
}

void AvatarManager::SpawnGroundBlood(float x, float y)
{
    // Generate 3-6 large stains
    int count = 3 + (std::rand() % 4);
    
    for (int i = 0; i < count; ++i)
    {
        GroundBlood b;
        // Random offset
        float ox = (static_cast<float>(std::rand() % 60) - 30.f);
        float oy = (static_cast<float>(std::rand() % 60) - 30.f);
        b.pos = {x + ox, y + oy};
        
        // Random scale
        b.scale = 1.0f + (static_cast<float>(std::rand() % 150) / 100.f);
        
        // Random rotation
        b.rotation = static_cast<float>(std::rand() % 360);
        
        // Alpha & Decay
        b.alpha = 180.f + (std::rand() % 50);
        b.decaySpeed = 5.f + (std::rand() % 5); // Lasts 20-30s
        
        // Color
        int rVar = std::rand() % 50;
        b.color = sf::Color(100 + rVar, 0, 0); // Darker red on floor
        
        m_groundBlood.push_back(b);
    }
}

void AvatarManager::ResetAllSpectators()
{
    m_localAvatar.SetSpectator(false);
    for (auto& pair : m_remoteAvatars)
    {
        pair.second.SetSpectator(false);
    }
}
