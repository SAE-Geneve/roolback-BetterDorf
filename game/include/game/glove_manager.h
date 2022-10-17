#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
/**
 * \brief A glove is a struct that keeps track of the player's punching glovesl. It handles states such as punching.
 */
struct Glove
{
    float punchingTime = 0.0f;
    bool isPunching = false;
    // Multiply the direction of the desired point, used to differentiate between the two fists
    float sign = 0.0f;
    PlayerNumber playerNumber = INVALID_PLAYER;
};

class GameManager;

/**
 * \brief GloveManager is a ComponentManager that holds all the gloves and handles their behaviour
 */
class GloveManager : public core::ComponentManager<Glove, static_cast<core::EntityMask>(ComponentType::GLOVE)>
{
public:
    explicit GloveManager(core::EntityManager& entityManager, GameManager& gameManager);
    void FixedUpdate(sf::Time dt);
private:
    GameManager& gameManager_;
};
}
