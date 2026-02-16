#pragma once
#include "Avatars/PlayerAvatar.h"
#include "UI/ChatBox.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <map>

class ClientContext;


class AvatarManager
{
public:
    explicit AvatarManager(ClientContext& ctx);

    // Called once after login
    void InitLocalAvatar(const std::string& pseudo);

    // Remote avatar management (called from network handlers)
    void AddRemoteAvatar(const std::string& pseudo, sf::Color color);
    void RemoveRemoteAvatar(const std::string& pseudo);
    void SetAvatarTarget(const std::string& pseudo, float x, float y);
    void SetLocalTarget(float x, float y);
    void SetSpectator(const std::string& pseudo, bool isSpectator);
    void ResetAllSpectators();

    // Per-frame update
    void Update(float dt, bool canInput);
    void SendInput();
    void BuildInteractionZones();
    void CheckInteraction(float dt, const ChatBox& chat);

    // Rendering
    void Draw(sf::Font& font);

    // Push feedback dispatch
    void OnPushAction(const std::string& pusherPseudo);

    // Accessors
    float GetInteractionTimer() const { return m_interactionTimer; }
    sf::Vector2f GetLocalAvatarPosition() const { return m_localAvatar.GetPosition(); }

private:
    ClientContext& m_ctx;

    PlayerAvatar m_localAvatar;
    std::map<std::string, PlayerAvatar> m_remoteAvatars;
    std::vector<InteractionZone> m_interactionZones;

    float m_interactionTimer = 0.f;
    bool m_wasEPressed = false;
    sf::Clock m_inputSendClock;
    sf::Clock m_dtClock;

    // Screen shake
    float m_shakeIntensity = 0.f;
    float m_shakeTimer = 0.f;
};
