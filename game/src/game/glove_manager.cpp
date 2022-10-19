#include "game/glove_manager.h"

#include "game/game_manager.h"
#include "game/physics_manager.h"


game::GloveManager::GloveManager(core::EntityManager& entityManager, PhysicsManager& physicsManager,
	GameManager& gameManager) : ComponentManager(entityManager), gameManager_(gameManager), physicsManager_(physicsManager)
{
}

void game::GloveManager::FixedUpdate(const sf::Time dt)
{
	// Loop over each player
	for (uint8_t playerNum = 0; playerNum < maxPlayerNmb ; playerNum++)
	{
		const core::Entity playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNum);
		Body playerBody = physicsManager_.GetBody(playerEntity);

		// Update both gloves
		for (const core::Entity& gloveEntity : gameManager_.GetGlovesEntityFromPlayerNumber(playerNum))
		{
			Glove glove = GetComponent(gloveEntity);
			Body gloveBody = physicsManager_.GetBody(gloveEntity);

			core::Vec2f relativeUp = core::Vec2f::up().Rotate(-playerBody.rotation);
			// Get the absolute point where the glove should try to be
			const core::Vec2f goalPos = playerBody.position + (relativeUp * gloveIdealDist).Rotate(gloveIdealAngle * glove.sign);

			if (glove.punchingTime >= 0.0f)
			{
				glove.punchingTime -= dt.asSeconds();
			}

			if (glove.isPunching)
			{
				if (glove.punchingTime <= 0.0f)
				{
					if (glove.hasLaunched)
					{
						// Stop the glove
						glove.isPunching = false;
						glove.hasLaunched = false;
						
						gloveBody.velocity = core::Vec2f::zero();

						auto col = physicsManager_.Getcol(gloveEntity);
						col.isTrigger = false;
						physicsManager_.SetCol(gloveEntity, col);
					}
					else
					{
						// Launch the glove
						gloveBody.velocity += relativeUp * punchingSpeed;
						glove.hasLaunched = true;

						glove.punchingTime = punchingTime;
					}
				}
			}
			else
				// Apply constraints
			{
				core::Vec2f toGlove = gloveBody.position - playerBody.position;

				// Project the glove against the ring formed by the min and max circles
				if (const float toGloveLength = toGlove.GetMagnitude(); toGloveLength > gloveMaxDist)
				{
					gloveBody.position = playerBody.position + toGlove.GetNormalized() * gloveMaxDist;
					toGlove = gloveBody.position -playerBody.position;
				}
				else if (toGloveLength < gloveMinDist)
				{
					gloveBody.position = playerBody.position + toGlove.GetNormalized() * gloveMinDist;
					toGlove = gloveBody.position - playerBody.position;
				}

				// Force the glove to be in a certain sector of the bounding ring

				core::Degree angleWithUp = core::GetPosAngle(core::Atan2(toGlove.y, toGlove.x)
					- core::Atan2(relativeUp.y, relativeUp.x));

				// Set the correct bounds for the glove
				const core::Degree bound1 = core::GetPosAngle(glove.sign >= 1.0f ? gloveAngle1 : gloveAngle2 * glove.sign);
				const core::Degree bound2 = core::GetPosAngle(glove.sign >= 1.0f ? gloveAngle2 : gloveAngle1 * glove.sign);

				// Check if outstide sector
				if (core::GetPosAngle(bound2 - bound1).value()
					< core::GetPosAngle(angleWithUp - bound1).value())
				{
					// Reset velocity
					gloveBody.velocity = core::Vec2f::zero();

					// Find which bound is the closest
					float dist1 = core::GetPosAngle(angleWithUp - bound1).value();
					float dist2 = core::GetPosAngle(angleWithUp - bound2).value();
					float constexpr halfCircle = 180.0f;
					if (dist1 > halfCircle)
					{
						dist1 = 2.0f * halfCircle - dist1;
					}
					if (dist2 > halfCircle)
					{
						dist2 = 2.0f * halfCircle - dist2;
					}

					// rotate glove so that it has a correct angle
					if (dist1 < dist2)
					{
						gloveBody.position = playerBody.position + relativeUp.Rotate(bound1) * toGlove.GetMagnitude();
					}
					else
					{
						gloveBody.position = playerBody.position + relativeUp.Rotate(bound2) * toGlove.GetMagnitude();
					}
				}

				// Apply force to get to desired point using seeking behaviour
				const auto toPoint = goalPos - gloveBody.position;
				const auto toVelocity = toPoint - gloveBody.velocity;
				gloveBody.velocity += toVelocity * gloveHoverSpeed * toPoint.GetMagnitude() / gloveDistSpeedBoostDist * dt.asSeconds();
			}

			// Apply the component changes
			physicsManager_.SetBody(gloveEntity, gloveBody);
			SetComponent(gloveEntity, glove);
		}
	}
}
