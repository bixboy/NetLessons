#include "../public/ChatBox.h"
#include <iostream>

ChatBox::ChatBox(): m_font(nullptr), m_isTyping(false), m_maxMessages(10), m_lineHeight(20.f)
{
}

void ChatBox::Setup(sf::Font& font, sf::Vector2f position, sf::Vector2f size)
{
    m_font = &font;

    m_bg.setPosition(position);
    m_bg.setSize(size);
    m_bg.setFillColor(sf::Color(0, 0, 0, 150));
    m_bg.setOutlineColor(sf::Color(100, 100, 100));
    m_bg.setOutlineThickness(1);

    float inputHeight = 30.f;
    m_inputBg.setPosition({position.x, position.y + size.y + 5});
    m_inputBg.setSize({size.x, inputHeight});
    m_inputBg.setFillColor(sf::Color(20, 20, 20, 200));
    m_inputBg.setOutlineColor(sf::Color::White);
    m_inputBg.setOutlineThickness(1);

    m_inputText.emplace(font);
    m_inputText->setCharacterSize(16);
    m_inputText->setFillColor(sf::Color::White);
    m_inputText->setPosition({position.x + 5, position.y + size.y + 10});
}

void ChatBox::AddMessage(const std::string& sender, const std::string& content, sf::Color color)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_font) 
        return;

    ChatMessage msg(*m_font);
    
    msg.textObj.setCharacterSize(16);
    msg.textObj.setFillColor(color);
    
    std::string fullText = (sender.empty() ? "" : "[" + sender + "] ") + content;
    msg.textObj.setString(fullText);
    
    m_messages.push_back(std::move(msg));
    
    if (m_messages.size() > m_maxMessages)
    {
        m_messages.pop_front();
    }

    UpdateLayout();
}

void ChatBox::UpdateLayout()
{
    float startY = m_bg.getPosition().y + m_bg.getSize().y - 20.f;
    
    int index = 0;
    for (auto it = m_messages.rbegin(); it != m_messages.rend(); ++it)
    {
        float y = startY - (index * m_lineHeight);
        it->textObj.setPosition({m_bg.getPosition().x + 5, y});
        index++;
    }
}

void ChatBox::HandleInput(const sf::Event& event)
{
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        if (keyEvent->code == sf::Keyboard::Key::Space && !m_isTyping)
        {
            m_isTyping = true;
            m_inputBg.setOutlineColor(sf::Color::Cyan);
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
            m_inputBg.setOutlineColor(sf::Color(100, 100, 100));
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
                if (m_currentInput.size() < 60)
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
            window.draw(msg.textObj);
        }
    }

    window.draw(m_inputBg);
    if ((m_isTyping || !m_currentInput.empty()) && m_inputText)
    {
        window.draw(*m_inputText);
    }
    else if (m_font)
    {
        sf::Text ph(*m_font);
        ph.setString("Appuyez sur SpaceBare pour chatter...");
        ph.setCharacterSize(14);
        ph.setFillColor(sf::Color(150, 150, 150));
        ph.setStyle(sf::Text::Italic);
        ph.setPosition({m_inputBg.getPosition().x + 5, m_inputBg.getPosition().y + 8});
        window.draw(ph);
    }
}

void ChatBox::SetOnSendMessage(std::function<void(const std::string&)> callback)
{
    m_onSend = callback;
}
