#include <game/player_character.h>
#include <game/game_manager.h>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif
namespace game
{
PlayerCharacterManager::PlayerCharacterManager(core::EntityManager& entityManager, PhysicsManager& physicsManager,
    GameManager& gameManager, GloveManager& gloveManager) :
    ComponentManager(entityManager),
    physicsManager_(physicsManager),
    gameManager_(gameManager),
    gloveManager_(gloveManager)
{
}

void PlayerCharacterManager::FixedUpdate(sf::Time dt)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
    {
        const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
        if (!entityManager_.HasComponent(playerEntity,
            static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)))
            continue;
        auto playerBody = physicsManager_.GetBody(playerEntity);
        auto playerCharacter = GetComponent(playerEntity);
        const auto input = playerCharacter.input;

        const bool right = input & PlayerInputEnum::PlayerInput::RIGHT;
        const bool left = input & PlayerInputEnum::PlayerInput::LEFT;
        const bool up = input & PlayerInputEnum::PlayerInput::UP;
        const bool down = input & PlayerInputEnum::PlayerInput::DOWN;

        const auto rotation = ((left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f)) * playerRotationalSpeed * dt.asSeconds();
        playerBody.rotation += rotation;

        const auto dir = core::Vec2f::up().Rotate(-playerBody.rotation);

        const auto speed = ((down ? -1.0f : 0.0f) + (up ? 1.0f : 0.0f)) * dir * playerSpeed;

        const auto originalVel = playerBody.velocity;
        playerBody.velocity += speed * dt.asSeconds();

        // Reduce velocity to match max speed if player isn't being knocked back
        if (playerCharacter.knockBackTime <= 0.0f && playerBody.velocity != core::Vec2f::zero())
        {
            auto normalizedVel = playerBody.velocity.GetNormalized();
        	if (playerBody.velocity.GetSqrMagnitude() > playerMaxSqrSpeed)
        	{
        		playerBody.velocity = normalizedVel * playerMaxSpeed;
        	}
            else if (!up && !down)
            {
                playerBody.velocity = normalizedVel * playerBody.velocity.GetMagnitude()
            		* (1.0f - playerFrictionLoss * dt.asSeconds());
            }
        }

        physicsManager_.SetBody(playerEntity, playerBody);

        // Change the associated gloves
        for (const core::Entity gloveEntity : gameManager_.GetGlovesEntityFromPlayerNumber(playerNumber))
        {
            if (gloveManager_.GetComponent(gloveEntity).isPunching)
            {
	            continue;
            }
            Body gloveBody = physicsManager_.GetBody(gloveEntity);

            // Rotate around player
            gloveBody.rotation = playerBody.rotation;

            const auto toGlove = gloveBody.position - playerBody.position;
            gloveBody.position = playerBody.position + toGlove.Rotate(rotation);
            gloveBody.velocity = gloveBody.velocity.Rotate(rotation);

            // Add the change in velocity
            gloveBody.velocity += playerBody.velocity - originalVel;

            physicsManager_.SetBody(gloveEntity, gloveBody);
        }

        if (playerCharacter.invincibilityTime > 0.0f)
        {
            playerCharacter.invincibilityTime -= dt.asSeconds();
            SetComponent(playerEntity, playerCharacter);
        }
        if (playerCharacter.knockBackTime > 0.0f)
        {
            playerCharacter.knockBackTime -= dt.asSeconds();
        }
    }
}
}
