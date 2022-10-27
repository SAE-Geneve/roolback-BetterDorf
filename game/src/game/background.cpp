#include "game/background.h"

#include "game/game_globals.h"
#include "SFML/Window/Window.hpp"
#include "utils/log.h"

void game::Background::Init(sf::Vector2u windowSize)
{
    if (!stageTxt_.loadFromFile("data/sprites/Stage.png"))
    {
        core::LogError("Could not load stage sprite");
    }
    if (!tilesTxt_.loadFromFile("data/sprites/Spike.png"))
    {
        core::LogError("Could not load tile sprite");
    }

    // Make the spike tiles
    for (unsigned x = 0; x < windowSize.x; x += tilesTxt_.getSize().x)
    {
        for (unsigned y = 0; y < windowSize.y; y += tilesTxt_.getSize().y)
        {
            sf::Sprite tile;
            tile.setTexture(tilesTxt_);
            tile.setPosition(x, y);

            tiles_.emplace_back(tile);
        }
    }

    // Make the stage
    stage_.setTexture(stageTxt_);
    stage_.setOrigin(static_cast<sf::Vector2f>(stageTxt_.getSize()) / 2.0f);
    const auto size = stageTxt_.getSize();

    stage_.setScale({ battleStagewidth / static_cast<float>(size.x) * core::pixelPerMeter,
        battleStageHeight / static_cast<float>(size.y) * core::pixelPerMeter });

    stage_.setPosition(windowSize.x / 2.0f,windowSize.y / 2.0f);


}

void game::Background::Draw(sf::RenderTarget& renderTarget)
{
    for(auto& tile : tiles_)
    {
        renderTarget.draw(tile);
    }

    renderTarget.draw(stage_);
}