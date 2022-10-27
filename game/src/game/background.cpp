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
    if (!tilesTxts_[0].loadFromFile("data/sprites/Spike.png") ||
        !tilesTxts_[1].loadFromFile("data/sprites/SpikeSplatter.png") ||
        !tilesTxts_[2].loadFromFile("data/sprites/SpikeMin.png") ||
        !tilesTxts_[3].loadFromFile("data/sprites/Spike2.png") ||
        !tilesTxts_[4].loadFromFile("data/sprites/Spike3.png"))
    {
        core::LogError("Could not load tile sprite");
    }

    // Make the spike tiles
    sf::Vector2u tileSize = tilesTxts_[0].getSize();
    for (unsigned x = 0; x < windowSize.x; x += tileSize.x)
    {
        for (unsigned y = 0; y < windowSize.y; y += tileSize.y)
        {
            sf::Sprite tile;
            tile.setTexture(tilesTxts_[std::rand() % tilesNum_]);
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