#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <functional>
#include <mutex>
#include <optional>


class ChatBox
{
public:
    ChatBox();
    
    void Setup(sf::Font& font, sf::Vector2f position, sf::Vector2f size);
    
    void HandleInput(const sf::Event& event);
    
    void Draw(sf::RenderWindow& window);
    
    void AddMessage(const std::string& sender, const std::string& content, sf::Color color = sf::Color::White);
    
    void SetOnSendMessage(std::function<void(const std::string&)> callback);

    bool IsTyping() const { return m_isTyping; }

private:
    struct ChatMessage
    {
        ChatMessage(const sf::Font& font) : textObj(font), lifetime(0) {}
        sf::Text textObj;
        float lifetime; 
    };

    void UpdateLayout();

    // UI
    sf::Font* m_font;
    sf::RectangleShape m_bg;
    sf::RectangleShape m_inputBg;
    
    std::optional<sf::Text> m_inputText;
    
    // Data
    std::deque<ChatMessage> m_messages;
    std::string m_currentInput;
    bool m_isTyping;
    
    // Paramètres
    int m_maxMessages;
    float m_lineHeight;
    
    std::function<void(const std::string&)> m_onSend;
    std::mutex m_mutex;
};
