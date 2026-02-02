#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <mutex>
#include <optional>

enum class MessageType
{
    Normal,     // Message de joueur
    System,     // Evenement systeme (join/leave)
    Info,       // Information du jeu
    Error,      // Erreur
    Success,    // Victoire
    Whisper     // Message privé
};

class ChatBox
{
public:
    ChatBox();
    
    void Setup(sf::Font& font, sf::Vector2f position, sf::Vector2f size);
    void SetPosition(sf::Vector2f position, sf::Vector2f size);
    
    void HandleInput(const sf::Event& event);
    
    void Draw(sf::RenderWindow& window);
    
    void AddChannel(const std::string& name);
    void AddMessage(const std::string& channel, const std::string& sender, const std::string& content, MessageType type, std::optional<sf::Color> customColor = std::nullopt);
    


    void SetActiveChannel(const std::string& name);
    std::string GetActiveChannel() const { return m_activeChannel; }
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
        std::string channel;
    };

    struct ChatTab
    {
        std::string name;
        std::deque<ChatMessage> messages;
        bool hasUnread = false;
    };

    void UpdateLayout();
    sf::Color GetColorForType(MessageType type) const;
    std::string GetPrefixForType(MessageType type) const;

    // UI
    sf::Font* m_font;
    sf::RectangleShape m_bg;
    sf::RectangleShape m_inputBg;
    sf::RectangleShape m_tabBg;
    sf::RectangleShape m_minimizeBtn;
    std::optional<sf::Text> m_minimizeText;
    bool m_isMinimized = false;
    sf::Vector2f m_position;
    sf::Vector2f m_size;
    
    std::optional<sf::Text> m_inputText;
    
    // Data
    std::vector<ChatTab> m_tabs;
    std::string m_activeChannel = "Global";
    std::string m_currentInput;
    bool m_isTyping;
    
    // Animation
    float m_cursorBlink;
    sf::Clock m_blinkClock;
    
    // Paramètres
    int m_maxMessages;
    int m_scrollOffset;
    float m_lineHeight;
    
    std::function<void(const std::string&)> m_onSend;
    std::mutex m_mutex;
};
