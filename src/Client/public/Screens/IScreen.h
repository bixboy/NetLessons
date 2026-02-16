#pragma once
#include <SFML/Graphics.hpp>


class IScreen
{
public:
    virtual ~IScreen() = default;
    virtual void HandleInput(const sf::Event& event) = 0;
    virtual void Draw() = 0;
};
