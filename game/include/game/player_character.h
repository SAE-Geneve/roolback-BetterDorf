#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
class PhysicsManager;
class GloveManager;

/**
 * \brief PlayerCharacter is a struct that holds information about the player character (when they can shoot again, their current input, and their current health).
 */
struct PlayerCharacter
{
    float knockBackTime = 0.0f;
    PlayerInput input = 0u;
    PlayerNumber playerNumber = INVALID_PLAYER;
    float damagePercent = 0.0f;
    float invincibilityTime = 0.0f;
};
class GameManager;

/**
 * \brief PlayerCharacterManager is a ComponentManager that holds all the PlayerCharacter in the game.
 */
class PlayerCharacterManager : public core::ComponentManager<PlayerCharacter, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)>
{
public:
    explicit PlayerCharacterManager(core::EntityManager& entityManager, PhysicsManager& physicsManager, GameManager& gameManager, GloveManager& gloveManager);
    void FixedUpdate(sf::Time dt);

private:
    PhysicsManager& physicsManager_;
    GameManager& gameManager_;
    GloveManager& gloveManager_;
};
}
