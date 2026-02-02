#include "ChatBox.h"
#include <iostream>
#include <cmath>
#include <algorithm>

ChatBox::ChatBox() : m_font(nullptr)
    , m_isTyping(false)
    , m_maxMessages(50)
    , m_lineHeight(22.f)
    , m_cursorBlink(0.f)
    , m_scrollOffset(0)
{
    AddChannel("Global");
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

    // Minimize Button
    float btnSize = 20.f;
    m_minimizeBtn.setSize({btnSize, btnSize});
    
    float btnY = m_isMinimized ? (position.y + size.y - btnSize) : (position.y - 25.f);
    m_minimizeBtn.setPosition({position.x + size.x - btnSize - 5.f, btnY});

    m_minimizeBtn.setFillColor(sf::Color(50, 50, 70));
    m_minimizeBtn.setOutlineColor(sf::Color(100, 100, 120));
    m_minimizeBtn.setOutlineThickness(1.f);

    if (m_font) 
    {
        m_minimizeText.emplace(*m_font);
        m_minimizeText->setString(m_isMinimized ? "+" : "-");
        m_minimizeText->setCharacterSize(16);
        m_minimizeText->setFillColor(sf::Color::White);
        m_minimizeText->setPosition({m_minimizeBtn.getPosition().x + 5.f, m_minimizeBtn.getPosition().y - 2.f});
    }

    UpdateLayout();
}

sf::Color ChatBox::GetColorForType(MessageType type) const
{
    switch (type)
    {
        case MessageType::System: 
            return sf::Color(255, 200, 100);  // Orange
        
        case MessageType::Info:    
            return sf::Color(100, 200, 255);  // Bleu
        
        case MessageType::Error:   
            return sf::Color(255, 100, 100);  // Rouge
        
        case MessageType::Success: 
            return sf::Color(100, 255, 150);  // Vert

        case MessageType::Whisper:
            return sf::Color(255, 100, 255);  // Magenta
        
        default:                  
            return sf::Color(220, 220, 220);  // Blanc
    }
}

std::string ChatBox::GetPrefixForType(MessageType type) const
{
    switch (type)
    {
        case MessageType::System:
            return "[SYS] ";
        
        case MessageType::Info:
            return "[INF] ";
        
        case MessageType::Error:
            return "[ERR] ";
        
        case MessageType::Success:
            return "[OK] ";

        case MessageType::Whisper:
            return "[PRV] ";
        
        default:
            return "";
    }
}

void ChatBox::AddChannel(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& t : m_tabs)
    {
        if (t.name == name) 
            return;
    }
    
    m_tabs.push_back({name, {}, false});
}

void ChatBox::SetActiveChannel(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeChannel = name;
    
    for (auto& t : m_tabs) 
    {
        if (t.name == name) 
            t.hasUnread = false;
    }
    
    m_scrollOffset = 0;
    UpdateLayout();
}

void ChatBox::AddMessage(const std::string& channel, const std::string& sender, const std::string& content, MessageType type, std::optional<sf::Color> customColor)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_font)
        return;

    auto CreateMsg = [&](const std::string& chan) 
    {
        ChatMessage msg(*m_font);
        msg.type = type;
        msg.channel = chan;
        
        sf::Color color = GetColorForType(type);
        std::string prefix = GetPrefixForType(type);
        
        msg.textObj.setCharacterSize(14);
        msg.textObj.setFillColor(color);
        
        if (type == MessageType::Normal && !sender.empty())
        {
            msg.prefixObj.setCharacterSize(14);
            msg.prefixObj.setFillColor(customColor.value_or(sf::Color(130, 180, 255)));
            msg.prefixObj.setStyle(sf::Text::Bold);
            msg.prefixObj.setString("[" + sender + "] ");
            msg.textObj.setString(content);
        }
        else if (type != MessageType::Normal)
        {
            msg.prefixObj.setCharacterSize(14);
            msg.prefixObj.setFillColor(color);
            msg.prefixObj.setString(prefix);
            if (!sender.empty() && sender != "SYSTEM")
            {
                if (type == MessageType::Whisper)
                    msg.textObj.setString(sender + " : " + content);
                else
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
        
        return msg;
    };

    bool channelFound = false;
    for (auto& tab : m_tabs)
    {
        if (tab.name == channel)
        {
            tab.messages.push_back(CreateMsg(channel));
            if (tab.messages.size() > m_maxMessages)
                tab.messages.pop_front();
            
            if (channel != m_activeChannel) 
                tab.hasUnread = true;
            
            channelFound = true;
            break;
        }
    }
    
    if (!channelFound && !channel.empty())
    {
        m_tabs.push_back({channel, {}, true});
        m_tabs.back().messages.push_back(CreateMsg(channel));
    }

    // Auto-route Global
    if (channel != "Global")
    {
        for (auto& tab : m_tabs)
        {
            if (tab.name == "Global")
            {
                ChatMessage globalCopy = CreateMsg(channel);
                tab.messages.push_back(std::move(globalCopy));
                
                if (tab.messages.size() > m_maxMessages) 
                    tab.messages.pop_front();
            }
        }
    }

    // Auto-route System
    if (type == MessageType::System && channel != "System")
    {
        for (auto& tab : m_tabs)
        {
            if (tab.name == "System")
            {
                tab.messages.push_back(CreateMsg(channel));
                
                if (tab.messages.size() > m_maxMessages) 
                    tab.messages.pop_front();
                
                if (m_activeChannel != "System") 
                    tab.hasUnread = true;
                
                break;
            }
        }
    }

    UpdateLayout();
}



void ChatBox::UpdateLayout()
{
    float startY = m_bg.getPosition().y + m_bg.getSize().y - 25.f;
    float baseX = m_bg.getPosition().x + 8;
    
    // Active tab
    ChatTab* activeTab = nullptr;
    for (auto& t : m_tabs) 
    {
        if (t.name == m_activeChannel) 
        {
            activeTab = &t;
            break;
        }
    }
    
    if (!activeTab) 
        return;

    int index = 0;
    
    if (m_scrollOffset < 0) 
        m_scrollOffset = 0;
    
    if (m_scrollOffset > (int)activeTab->messages.size() - 5) 
         m_scrollOffset = std::max(0, (int)activeTab->messages.size() - 5);

    auto itStart = activeTab->messages.rbegin() + m_scrollOffset;
    auto itEnd = activeTab->messages.rend();

    for (auto it = itStart; it != itEnd; ++it)
    {
        float y = startY - (index * m_lineHeight);
        
        if (y < m_bg.getPosition().y) 
            break;
        
        it->prefixObj.setPosition({baseX, y});
        
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
    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouseEvent->button == sf::Mouse::Button::Left)
        {
            float mx = static_cast<float>(mouseEvent->position.x);
            float my = static_cast<float>(mouseEvent->position.y);
            
            float tabX = m_bg.getPosition().x;
            float tabY = m_bg.getPosition().y - 25.f; 
            
            if (m_font)
            {
                sf::FloatRect btnBounds = m_minimizeBtn.getGlobalBounds();
                if (btnBounds.contains(sf::Vector2f(mx, my)))
                {
                    m_isMinimized = !m_isMinimized;
                    if(m_minimizeText)
                        m_minimizeText->setString(m_isMinimized ? "+" : "-");
                    
                    if (m_isMinimized) 
                        m_isTyping = false;
                    
                    SetPosition(m_position, m_size);
                    return;
                }

                if (m_isMinimized) 
                    return;
                
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& tab : m_tabs)
                {
                    sf::Text temp(*m_font);
                    temp.setString(tab.name);
                    temp.setCharacterSize(12);
                    float tabW = temp.getLocalBounds().size.x + 20.f;
                    
                    if (mx >= tabX && mx <= tabX + tabW && my >= tabY && my <= tabY + 25.f)
                    {
                        m_activeChannel = tab.name;
                        for(auto& t : m_tabs)
                        {
                             if(t.name == tab.name) 
                                 t.hasUnread = false;
                        }
                        
                        UpdateLayout();
                        return;
                    }
                    tabX += tabW + 2.f;
                }
            }
        }
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>())
    {
        // Activation chat
        if (keyEvent->code == sf::Keyboard::Key::Tab && !m_isTyping)
        {
            m_isTyping = true;
            m_inputBg.setOutlineColor(sf::Color(0, 200, 255));
            m_inputBg.setFillColor(sf::Color(30, 30, 50, 250));
            m_blinkClock.restart();
            return;
        }

        // Annulation
        if (keyEvent->code == sf::Keyboard::Key::Escape && m_isTyping)
        {
            m_isTyping = false;
            m_currentInput.clear();
            if (m_inputText)
                m_inputText->setString("");
            
            m_inputBg.setOutlineColor(sf::Color(60, 60, 80));
            m_inputBg.setFillColor(sf::Color(25, 25, 35, 240));
            return;
        }

        // Envoi
        if (keyEvent->code == sf::Keyboard::Key::Enter && m_isTyping)
        {
            if (!m_currentInput.empty())
            {
                if (m_activeChannel == "System" && m_currentInput[0] != '/')
                {
                    AddMessage("System", "", "Commands only (/...) in System channel.", MessageType::Error);
                    m_currentInput.clear();
                    if (m_inputText)
                        m_inputText->setString("");
                    
                    return;
                }

                if (m_onSend) 
                    m_onSend(m_currentInput);
                
                m_currentInput.clear();
                
                if (m_inputText)
                    m_inputText->setString("");
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
                if (m_inputText) 
                    m_inputText->setString(m_currentInput);
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

    // SCROLL
    if (const auto* wheelEvent = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        if (wheelEvent->wheel == sf::Mouse::Wheel::Vertical)
        {
            float mx = (float)wheelEvent->position.x;
            float my = (float)wheelEvent->position.y;
            
            sf::FloatRect bounds = m_bg.getGlobalBounds();
            if (bounds.contains({mx, my}))
            {
                if (wheelEvent->delta > 0) // Up
                    m_scrollOffset++;
                else // Down
                    m_scrollOffset--;
                
                if (m_scrollOffset < 0) m_scrollOffset = 0;
                
                UpdateLayout();
            }
        }
    }
}

void ChatBox::Draw(sf::RenderWindow& window)
{
    std::scoped_lock lock(m_mutex);

    // draw minimize
    window.draw(m_minimizeBtn);
    if(m_minimizeText) 
        window.draw(*m_minimizeText);

    if (m_isMinimized) 
        return;

    // Draw Tabs
    float tabX = m_bg.getPosition().x;
    float tabY = m_bg.getPosition().y - 25.f;
    
    for (const auto& tab : m_tabs)
    {
        bool isActive = (tab.name == m_activeChannel);
        
        sf::Text tabText(*m_font);
        tabText.setString(tab.name);
        tabText.setCharacterSize(12);
        tabText.setFillColor(isActive ? sf::Color::White : (tab.hasUnread ? sf::Color(255, 200, 100) : sf::Color(150, 150, 150)));
        tabText.setPosition({tabX + 10.f, tabY + 4.f});
        
        if (isActive) 
            tabText.setStyle(sf::Text::Bold);

        float tabW = tabText.getLocalBounds().size.x + 20.f;
        sf::RectangleShape tabRect({tabW, 25.f});
        tabRect.setPosition({tabX, tabY});
        tabRect.setFillColor(isActive ? sf::Color(40, 40, 60) : sf::Color(20, 20, 30));
        tabRect.setOutlineColor(sf::Color(60, 60, 80));
        tabRect.setOutlineThickness(1.f);
        
        window.draw(tabRect);
        window.draw(tabText);
        
        tabX += tabW + 2.f;
    }

    window.draw(m_bg);
    
    if (m_scrollOffset > 0)
    {
        sf::Text scrollInd(*m_font);
        scrollInd.setString("Scroll: " + std::to_string(m_scrollOffset));
        scrollInd.setCharacterSize(10);
        scrollInd.setFillColor(sf::Color(100, 100, 100));
        scrollInd.setPosition({m_bg.getPosition().x + m_bg.getSize().x - 60, m_bg.getPosition().y + 5});
        window.draw(scrollInd);
    }
    
    float minY = m_bg.getPosition().y;
    
    ChatTab* activeTab = nullptr;
    for (auto& t : m_tabs)
    {
        if (t.name == m_activeChannel) 
            activeTab = &t;
    }

    if (activeTab)
    {
        for (const auto& msg : activeTab->messages)
        {
            if (msg.textObj.getPosition().y >= minY)
            {
                if (!msg.prefixObj.getString().isEmpty())
                    window.draw(msg.prefixObj);
                
                window.draw(msg.textObj);
            }
        }
    }

    window.draw(m_inputBg);
    
    if (m_isTyping && m_inputText)
    {
        window.draw(*m_inputText);
        
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
