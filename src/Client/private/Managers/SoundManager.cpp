#include "Managers/SoundManager.h"
#include <iostream>
#include <algorithm>

SoundManager::SoundManager()
{
}

void SoundManager::Init()
{
    LoadSound(SoundType::Select, "assets/Sounds/ClickButton.mp3");
    LoadSound(SoundType::Lose,   "assets/Sounds/Lose.mp3");
    LoadSound(SoundType::Join,   "assets/Sounds/Join.mp3"); 
    LoadSound(SoundType::Leave,  "assets/Sounds/Leave.mp3");
}

void SoundManager::LoadSound(SoundType type, const std::string& path)
{
    AudioItem item;
    
    if (item.buffer->loadFromFile(path))
    {
        item.sound.setBuffer(*item.buffer);
        item.sound.setVolume(m_masterVolume);
        m_audioItems[type] = item;
    }
    else
    {
        std::cerr << "[SoundManager] Failed to load sound: " << path << std::endl;
        m_audioItems[type] = item; 
    }
}

void SoundManager::Play(SoundType type)
{
    if (m_isMuted) 
        return;

    auto it = m_audioItems.find(type);
    if (it != m_audioItems.end())
    {
        float pitch = 0.95f + static_cast<float>(rand() % 10) / 100.f;
        it->second.sound.setPitch(pitch);
        it->second.sound.play();
    }
}

void SoundManager::SetVolume(float volume)
{
    m_masterVolume = std::clamp(volume, 0.f, 100.f);
    for (auto& [type, item] : m_audioItems)
    {
        item.sound.setVolume(m_masterVolume);
    }
}

void SoundManager::SetMute(bool mute)
{
    m_isMuted = mute;
    if (m_isMuted)
    {
        for (auto& [type, item] : m_audioItems)
        {
            item.sound.stop();
        }
    }
}

