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
    PlayerNumber playerNumber = INVALID_PLAYER;
    // Multiply the direction of the desired point, used to differentiate between the two fists
    float sign = 0.0f;

    float punchingTime = 0.0f;
    float recoveryTime = 0.0f;

    bool isPunching = false;
    bool isRecovering = false;
    bool hasLaunched = false;

    core::Vec2f velFromPlayer = core::Vec2f::zero();
    // The position the glove is returning from (i.e where the glove ended up after a punch)
    core::Vec2f returningFromPos = core::Vec2f::zero();
};

class GameManager;
class PhysicsManager;

/**
 * \brief GloveManager is a ComponentManager that holds all the gloves and handles their behaviour
 */
class GloveManager : public core::ComponentManager<Glove, static_cast<core::EntityMask>(ComponentType::GLOVE)>
{
public:
    explicit GloveManager(core::EntityManager& entityManager,
        PhysicsManager& physicsManager, GameManager& gameManager);
    void FixedUpdate(sf::Time dt);

    void StartPunch(core::Entity gloveEntity);
    void StartReturn(core::Entity gloveEntity);
    void StartIdle(core::Entity gloveEntity);
private:
    GameManager& gameManager_;
    PhysicsManager& physicsManager_;
};
}
