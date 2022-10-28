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
    for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
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
        const std::array punch = { static_cast<bool>(input & PlayerInputEnum::PlayerInput::PUNCH),
            static_cast<bool>(input & PlayerInputEnum::PlayerInput::PUNCH2) };

        const auto rotation = ((left ? -1.0f : 0.0f) + (right ? 1.0f : 0.0f)) * PLAYER_ROTATIONAL_SPEED * dt.asSeconds();
        playerBody.rotation += rotation;

        const auto dir = core::Vec2f::up().Rotate(-playerBody.rotation);

        const auto speed = ((down ? -1.0f : 0.0f) + (up ? 1.0f : 0.0f)) * dir * PLAYER_SPEED;

        // Reduce velocity to match max speed if player isn't being knocked back
        if (playerCharacter.knockBackTime <= 0.0f)
        {
            playerBody.velocity += speed * dt.asSeconds();

            if (playerBody.velocity != core::Vec2f::zero())
            {
                if (playerBody.velocity.GetMagnitude() > PLAYER_MAX_SPEED)
                {
                    playerBody.velocity = playerBody.velocity.GetNormalized() * (PLAYER_MAX_SPEED - FLT_EPSILON);
                }
                else if (!up && !down)
                {
                    playerBody.velocity = playerBody.velocity * (1.0f - PLAYER_FRICTION_LOSS * dt.asSeconds());
                }
            }
        }

        physicsManager_.SetBody(playerEntity, playerBody);

    	// Update the associated gloves
        const auto gloves = gameManager_.GetGlovesEntityFromPlayerNumber(playerNumber);
        for (int i = 0; i < 2 ; i++)
        {
	        const auto gloveEntity = gloves[i];
            auto glove = gloveManager_.GetComponent(gloveEntity);

            if (glove.isPunching || glove.isRecovering)
            {
	            continue;
            }
            Body gloveBody = physicsManager_.GetBody(gloveEntity);

            // Rotate around player
            gloveBody.rotation = playerBody.rotation;

            const auto toGlove = gloveBody.position - playerBody.position;
            gloveBody.position = playerBody.position + toGlove.Rotate(-rotation); 
            //gloveBody.velocity = gloveBody.velocity.Rotate(-rotation);

            // Add the change in velocity
            gloveBody.velocity -= glove.velFromPlayer;
            glove.velFromPlayer = playerBody.velocity;
            gloveBody.velocity = gloveBody.velocity.Rotate(-rotation);
            gloveBody.velocity += playerBody.velocity;

            gloveManager_.SetComponent(gloveEntity, glove);

            physicsManager_.SetBody(gloveEntity, gloveBody);

            // Handle punching input
            if (punch[i])
            {
                gloveManager_.StartPunch(gloveEntity);
            }
        }

        if (playerCharacter.invincibilityTime > 0.0f)
        {
            playerCharacter.invincibilityTime -= dt.asSeconds();
        }
        if (playerCharacter.knockBackTime > 0.0f)
        {
            playerCharacter.knockBackTime -= dt.asSeconds();
        }
        SetComponent(playerEntity, playerCharacter);
    }
}
}
