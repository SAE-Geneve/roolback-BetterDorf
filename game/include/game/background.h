#pragma once
#include <array>
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

    void SetWindowSize(sf::Vector2u windowSize);
private:
    std::vector<sf::Sprite> tiles_;
    sf::Sprite stage_;

    sf::Texture stageTxt_;
	static constexpr int tilesNum_ = 5;
	std::array<sf::Texture, tilesNum_> tilesTxts_;

    int static constexpr SIDE_BUFFER = 15;
    bool isInit_ = false;
};
}
