#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
/**
 * \brief Bullet is a struct that contains info about a player bullet (when it will be destroyed and whose player it is).
 */
struct Glove
{
    float punchingTime = 0.0f;
    bool isPunching = false;

    PlayerNumber playerNumber = INVALID_PLAYER;
};

class GameManager;

/**
 * \brief BulletManager is a ComponentManager that holds all the Bullet in one place.
 * It will automatically destroy the Bullet when remainingTime is over.
 */
class GloveManager : public core::ComponentManager<Glove, static_cast<core::EntityMask>(ComponentType::BULLET)>
{
public:
    explicit GloveManager(core::EntityManager& entityManager, GameManager& gameManager);
    void FixedUpdate(sf::Time dt);
private:
    GameManager& gameManager_;
};
}
