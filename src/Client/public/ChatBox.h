#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <functional>
#include <mutex>
#include <optional>

enum class MessageType
{
    Normal,     // Message de joueur
    System,     // Événement système (join/leave)
    Info,       // Information du jeu
    Error,      // Erreur
    Success     // Victoire, etc.
};

class ChatBox
{
public:
    ChatBox();
    
    void Setup(sf::Font& font, sf::Vector2f position, sf::Vector2f size);
    void SetPosition(sf::Vector2f position, sf::Vector2f size);
    
    void HandleInput(const sf::Event& event);
    
    void Draw(sf::RenderWindow& window);
    
    void AddMessage(const std::string& sender, const std::string& content, sf::Color color = sf::Color::White);
    void AddMessage(const std::string& sender, const std::string& content, MessageType type);
    
    void SetOnSendMessage(std::function<void(const std::string&)> callback);

    bool IsTyping() const { return m_isTyping; }

private:
    struct ChatMessage
    {
        ChatMessage(const sf::Font& font) : textObj(font), prefixObj(font), lifetime(0), type(MessageType::Normal) {}
        sf::Text textObj;
        sf::Text prefixObj;
        MessageType type;
        float lifetime; 
    };

    void UpdateLayout();
    sf::Color GetColorForType(MessageType type) const;
    std::string GetPrefixForType(MessageType type) const;

    // UI
    sf::Font* m_font;
    sf::RectangleShape m_bg;
    sf::RectangleShape m_inputBg;
    sf::Vector2f m_position;
    sf::Vector2f m_size;
    
    std::optional<sf::Text> m_inputText;
    
    // Data
    std::deque<ChatMessage> m_messages;
    std::string m_currentInput;
    bool m_isTyping;
    
    // Animation
    float m_cursorBlink;
    sf::Clock m_blinkClock;
    
    // Paramètres
    int m_maxMessages;
    float m_lineHeight;
    
    std::function<void(const std::string&)> m_onSend;
    std::mutex m_mutex;
};
