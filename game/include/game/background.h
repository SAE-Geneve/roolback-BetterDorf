#pragma once
#include <vector>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "graphics/graphics.h"


namespace game
{
/**
 * \brief StarBackground is a drawable object that draws a starfield on a screen.
 */
class Background final : public core::DrawInterface
{
public:
    void Init(sf::Vector2u windowSize);
    void Draw(sf::RenderTarget& renderTarget) override;
private:
    std::vector<sf::Sprite> tiles_;
    sf::Sprite stage_;

    sf::Texture stageTxt_;
    sf::Texture tilesTxt_;
};
}
