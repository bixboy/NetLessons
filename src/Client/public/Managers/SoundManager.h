#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <vector>
#include <string>
#include <memory>

enum class SoundType
{
    Select,
    Win,
    Lose,
    Push,
    Join,
    Leave
};

class SoundManager
{
public:
    SoundManager();
    ~SoundManager() = default;

    void    Init();
    void    Play(SoundType type);
    
    // Global settings
    void SetVolume(float volume);
    void SetMute(bool mute);
    bool IsMuted() const { return m_isMuted; }

    // System Volume (Windows)
    void SetSystemVolume(float volume);
    void SetSystemMute(bool mute);

private:
    struct AudioItem
    {
        std::shared_ptr<sf::SoundBuffer> buffer;
        sf::Sound sound;

        AudioItem() : buffer(std::make_shared<sf::SoundBuffer>()), sound(*buffer) {}
    };

    void LoadSound(SoundType type, const std::string& path);

    std::map<SoundType, AudioItem> m_audioItems;
    float m_masterVolume = 100.f;
    bool m_isMuted = false;
};
