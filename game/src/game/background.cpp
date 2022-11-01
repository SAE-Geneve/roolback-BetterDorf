#include "game/background.h"

#include "game/game_globals.h"
#include "SFML/Window/Window.hpp"
#include "utils/log.h"

void game::Background::Init(sf::Vector2u windowSize)
{
    isInit_ = true;

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
    const sf::Vector2u tileSize = tilesTxts_[0].getSize();

    for (float x = 0 - SIDE_BUFFER; x < static_cast<int>(windowSize.x / tileSize.x) + SIDE_BUFFER; x++)
    {
        for (float y = 0 - SIDE_BUFFER; y < static_cast<int>(windowSize.y / tileSize.y) + SIDE_BUFFER; y++)
        {
            sf::Sprite tile;
            tile.setTexture(tilesTxts_[std::rand() % tilesNum_]);
            tile.setPosition(static_cast<float>(x) * static_cast<float>(tileSize.x), static_cast<float>(y) * static_cast<float>(tileSize.y));

            tiles_.emplace_back(tile);
        }
    }

    // Make the stage
    stage_.setTexture(stageTxt_);
    stage_.setOrigin(static_cast<sf::Vector2f>(stageTxt_.getSize()) / 2.0f);
    const auto size = stageTxt_.getSize();

    stage_.setScale({ BATTLE_STAGE_WIDTH / static_cast<float>(size.x) * core::pixelPerMeter,
        BATTLE_STAGE_HEIGHT / static_cast<float>(size.y) * core::pixelPerMeter });

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

void game::Background::SetWindowSize(sf::Vector2u windowSize)
{
    if(!isInit_)
    {
        return;
    }

    stage_.setPosition(windowSize.x / 2.0f, windowSize.y / 2.0f);
}
