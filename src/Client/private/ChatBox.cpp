#include "../public/ChatBox.h"
#include <iostream>

ChatBox::ChatBox()
    : m_font(nullptr)
    , m_isTyping(false)
    , m_maxMessages(10)
    , m_lineHeight(22.f)
    , m_cursorBlink(0.f)
{
}

void ChatBox::Setup(sf::Font& font, sf::Vector2f position, sf::Vector2f size)
{
    m_font = &font;
    m_position = position;
    m_size = size;

    SetPosition(position, size);

    m_inputText.emplace(font);
    m_inputText->setCharacterSize(14);
    m_inputText->setFillColor(sf::Color::White);
    m_inputText->setPosition({position.x + 10, position.y + size.y + 8});
}

void ChatBox::SetPosition(sf::Vector2f position, sf::Vector2f size)
{
    m_position = position;
    m_size = size;

    m_bg.setPosition(position);
    m_bg.setSize(size);
    m_bg.setFillColor(sf::Color(15, 15, 25, 220));
    m_bg.setOutlineColor(sf::Color(60, 60, 80));
    m_bg.setOutlineThickness(1);

    float inputHeight = 32.f;
    m_inputBg.setPosition({position.x, position.y + size.y + 5});
    m_inputBg.setSize({size.x, inputHeight});
    m_inputBg.setFillColor(sf::Color(25, 25, 35, 240));
    m_inputBg.setOutlineColor(sf::Color(60, 60, 80));
    m_inputBg.setOutlineThickness(1);

    if (m_inputText)
    {
        m_inputText->setPosition({position.x + 10, position.y + size.y + 12});
    }

    UpdateLayout();
}

sf::Color ChatBox::GetColorForType(MessageType type) const
{
    switch (type)
    {
        case MessageType::System:  return sf::Color(255, 200, 100);  // Orange doré
        case MessageType::Info:    return sf::Color(100, 200, 255);  // Bleu clair
        case MessageType::Error:   return sf::Color(255, 100, 100);  // Rouge
        case MessageType::Success: return sf::Color(100, 255, 150);  // Vert
        default:                   return sf::Color(220, 220, 220);  // Blanc cassé
    }
}

std::string ChatBox::GetPrefixForType(MessageType type) const
{
    switch (type)
    {
        case MessageType::System:  return "● ";
        case MessageType::Info:    return "◆ ";
        case MessageType::Error:   return "✖ ";
        case MessageType::Success: return "✔ ";
        default:                   return "";
    }
}

void ChatBox::AddMessage(const std::string& sender, const std::string& content, sf::Color color)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_font) 
        return;

    ChatMessage msg(*m_font);
    msg.type = MessageType::Normal;
    
    msg.textObj.setCharacterSize(14);
    msg.textObj.setFillColor(color);
    
    if (!sender.empty())
    {
        msg.prefixObj.setCharacterSize(14);
        msg.prefixObj.setFillColor(sf::Color(150, 150, 200));
        msg.prefixObj.setString("[" + sender + "] ");
        msg.textObj.setString(content);
    }
    else
    {
        msg.textObj.setString(content);
    }
    
    m_messages.push_back(std::move(msg));
    
    if (m_messages.size() > m_maxMessages)
    {
        m_messages.pop_front();
    }

    UpdateLayout();
}

void ChatBox::AddMessage(const std::string& sender, const std::string& content, MessageType type)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_font) 
        return;

    ChatMessage msg(*m_font);
    msg.type = type;
    
    sf::Color color = GetColorForType(type);
    std::string prefix = GetPrefixForType(type);
    
    msg.textObj.setCharacterSize(14);
    msg.textObj.setFillColor(color);
    
    if (type == MessageType::Normal && !sender.empty())
    {
        // Message de chat normal avec sender
        msg.prefixObj.setCharacterSize(14);
        msg.prefixObj.setFillColor(sf::Color(130, 180, 255));
        msg.prefixObj.setStyle(sf::Text::Bold);
        msg.prefixObj.setString("[" + sender + "] ");
        msg.textObj.setString(content);
    }
    else if (type != MessageType::Normal)
    {
        // Message système avec icône
        msg.prefixObj.setCharacterSize(14);
        msg.prefixObj.setFillColor(color);
        msg.prefixObj.setString(prefix);
        
        if (!sender.empty() && sender != "SYSTEM")
        {
            msg.textObj.setString(sender + " " + content);
        }
        else
        {
            msg.textObj.setString(content);
        }
    }
    else
    {
        msg.textObj.setString(content);
    }
    
    m_messages.push_back(std::move(msg));
    
    if (m_messages.size() > m_maxMessages)
    {
        m_messages.pop_front();
    }

    UpdateLayout();
}

void ChatBox::UpdateLayout()
{
    float startY = m_bg.getPosition().y + m_bg.getSize().y - 25.f;
    float baseX = m_bg.getPosition().x + 8;
    
    int index = 0;
    for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it)
    {
        float y = startY - (index * m_lineHeight);
        
        // Position du préfixe
        it->prefixObj.setPosition({baseX, y});
        
        // Position du texte après le préfixe
        float prefixWidth = 0.f;
        if (!it->prefixObj.getString().isEmpty())
        {
            prefixWidth = it->prefixObj.getLocalBounds().size.x + 2.f;
        }
        it->textObj.setPosition({baseX + prefixWidth, y});
        
        index++;
    }
}

void ChatBox::HandleInput(const sf::Event& event)
{
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        // Activation du chat avec TAB
        if (keyEvent->code == sf::Keyboard::Key::Tab && !m_isTyping)
        {
            m_isTyping = true;
            m_inputBg.setOutlineColor(sf::Color(0, 200, 255));
            m_inputBg.setFillColor(sf::Color(30, 30, 50, 250));
            m_blinkClock.restart();
            return;
        }

        // Annulation avec Escape
        if (keyEvent->code == sf::Keyboard::Key::Escape && m_isTyping)
        {
            m_isTyping = false;
            m_currentInput.clear();
            if (m_inputText) m_inputText->setString("");
            m_inputBg.setOutlineColor(sf::Color(60, 60, 80));
            m_inputBg.setFillColor(sf::Color(25, 25, 35, 240));
            return;
        }

        // Envoi
        if (keyEvent->code == sf::Keyboard::Key::Enter && m_isTyping)
        {
            if (!m_currentInput.empty())
            {
                if (m_onSend) m_onSend(m_currentInput);
                m_currentInput.clear();
                if (m_inputText) m_inputText->setString("");
            }
            m_isTyping = false;
            m_inputBg.setOutlineColor(sf::Color(60, 60, 80));
            m_inputBg.setFillColor(sf::Color(25, 25, 35, 240));
            return;
        }
        
        if (keyEvent->code == sf::Keyboard::Key::Backspace && m_isTyping)
        {
            if (!m_currentInput.empty())
            {
                m_currentInput.pop_back();
                if (m_inputText) m_inputText->setString(m_currentInput);
            }
        }
    }

    // Saisie texte
    if (m_isTyping)
    {
        if (const auto* textEvent = event.getIf<sf::Event::TextEntered>())
        {
            if (textEvent->unicode >= 32 && textEvent->unicode < 128)
            {
                if (m_currentInput.size() < 80)
                {
                    m_currentInput += static_cast<char>(textEvent->unicode);
                    if (m_inputText)
                        m_inputText->setString(m_currentInput);
                }
            }
        }
    }
}

void ChatBox::Draw(sf::RenderWindow& window)
{
    std::scoped_lock lock(m_mutex);

    window.draw(m_bg);
    
    float minY = m_bg.getPosition().y;
    
    for (const auto& msg : m_messages)
    {
        if (msg.textObj.getPosition().y >= minY)
        {
            // Dessiner le préfixe d'abord
            if (!msg.prefixObj.getString().isEmpty())
            {
                window.draw(msg.prefixObj);
            }
            window.draw(msg.textObj);
        }
    }

    window.draw(m_inputBg);
    
    if (m_isTyping && m_inputText)
    {
        window.draw(*m_inputText);
        
        // Curseur clignotant
        float blinkTime = m_blinkClock.getElapsedTime().asSeconds();
        if (fmod(blinkTime, 1.0f) < 0.5f)
        {
            sf::RectangleShape cursor({2.f, 16.f});
            float cursorX = m_inputText->getPosition().x + m_inputText->getLocalBounds().size.x + 2.f;
            cursor.setPosition({cursorX, m_inputText->getPosition().y + 2.f});
            cursor.setFillColor(sf::Color(0, 200, 255));
            window.draw(cursor);
        }
    }
    else if (m_font)
    {
        sf::Text ph(*m_font);
        ph.setString("Appuyez sur [TAB] pour chatter...");
        ph.setCharacterSize(12);
        ph.setFillColor(sf::Color(100, 100, 120));
        ph.setStyle(sf::Text::Italic);
        ph.setPosition({m_inputBg.getPosition().x + 10, m_inputBg.getPosition().y + 10});
        window.draw(ph);
    }
}

void ChatBox::SetOnSendMessage(std::function<void(const std::string&)> callback)
{
    m_onSend = callback;
}
