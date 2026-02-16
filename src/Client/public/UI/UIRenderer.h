#pragma once
#include <SFML/Graphics.hpp>
#include <string>

class ClientContext;


class UIRenderer
{
public:
    explicit UIRenderer(ClientContext& ctx);

    void DrawBackground();
    void DrawPanel(float x, float y, float w, float h, sf::Color color, float cornerRadius = 0.f);
    void DrawButton(const std::string& text, float y, bool active = false, bool hovered = false);
    void DrawInputBox(const std::string& label, const std::string& value, float y, bool active = true);
    void CenterText(sf::Text& text, float y, int fontSize);

private:
    ClientContext& m_ctx;
};
