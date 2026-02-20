#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <vector>
#include <string>
#include <memory>

enum class SoundType
{
    Select,
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



private:
    struct AudioItem
    {
        std::shared_ptr<sf::SoundBuffer> buffer;
        sf::Sound sound;

        AudioItem() : buffer(std::make_shared<sf::SoundBuffer>()), sound(*buffer) {}
    };

    void LoadSound(SoundType type, const std::string& path);

    std::map<SoundType, AudioItem> m_audioItems;
    float m_masterVolume = 70.f;
    bool m_isMuted = false;
};
