#include "game/glove_manager.h"

#include "game/game_manager.h"
#include "game/physics_manager.h"


game::GloveManager::GloveManager(core::EntityManager& entityManager, PhysicsManager& physicsManager,
	GameManager& gameManager) : ComponentManager(entityManager), gameManager_(gameManager),
	physicsManager_(physicsManager)
{
}

void game::GloveManager::FixedUpdate(const sf::Time dt)
{
	// Loop over each player
	for (uint8_t playerNum = 0; playerNum < MAX_PLAYER_NMB; playerNum++)
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
			const core::Vec2f goalPos = playerBody.position + (relativeUp * GLOVE_IDEAL_DIST).Rotate(GLOVE_IDEAL_ANGLE * glove.sign);

			if (glove.punchingTime >= 0.0f)
			{
				glove.punchingTime -= dt.asSeconds();
			}
			if (glove.recoveryTime >= 0.0f)
			{
				glove.recoveryTime -= dt.asSeconds();
			}

			if (glove.isPunching)
			{
				if (glove.isRecovering)
				{
					if (glove.recoveryTime > 0.0f)
					{
						float ratio = std::clamp((GLOVE_RECOVERY_TIME - glove.recoveryTime) / GLOVE_RECOVERY_TIME,
							0.0f, 1.0f);
						gloveBody.position = core::Vec2f::Lerp(glove.returningFromPos, goalPos, ratio);
					}
					else
					{
						StartIdle(gloveEntity);
						gloveBody = physicsManager_.GetBody(gloveEntity);
						gloveBody.position = goalPos;
						glove = GetComponent(gloveEntity);
					}
				}
				else if (glove.punchingTime <= 0.0f)
				{
					if (glove.hasLaunched)
					{
						StartReturn(gloveEntity);
						gloveBody = physicsManager_.GetBody(gloveEntity);
						glove = GetComponent(gloveEntity);
					}
					else
					{
						// Launch the glove
						gloveBody.velocity = relativeUp * PUNCHING_SPEED;
						glove.hasLaunched = true;

						glove.punchingTime = PUNCHING_TIME;
					}
				}
			}
			else
				// Apply constraints
			{
				core::Vec2f toGlove = gloveBody.position - playerBody.position;

				// Project the glove against the ring formed by the min and max circles
				if (const float toGloveLength = toGlove.GetMagnitude(); toGloveLength > GLOVE_MAX_DIST)
				{
					gloveBody.position = playerBody.position + toGlove.GetNormalized() * GLOVE_MAX_DIST;
					toGlove = gloveBody.position - playerBody.position;
				}
				else if (toGloveLength < GLOVE_MIN_DIST)
				{
					gloveBody.position = playerBody.position + toGlove.GetNormalized() * GLOVE_MIN_DIST;
					toGlove = gloveBody.position - playerBody.position;
				}

				// Force the glove to be in a certain sector of the bounding ring

				core::Degree angleWithUp = core::GetPosAngle(core::Atan2(toGlove.y, toGlove.x)
					- core::Atan2(relativeUp.y, relativeUp.x));

				// Set the correct bounds for the glove
				const core::Degree bound1 = core::GetPosAngle(glove.sign >= 1.0f ? GLOVE_ANGLE_1 : GLOVE_ANGLE_2 * glove.sign);
				const core::Degree bound2 = core::GetPosAngle(glove.sign >= 1.0f ? GLOVE_ANGLE_2 : GLOVE_ANGLE_1 * glove.sign);

				// Check if outstide sector
				if (core::GetPosAngle(bound2 - bound1).value()
					< core::GetPosAngle(angleWithUp - bound1).value())
				{
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
				const auto toVelocity = toPoint + playerBody.velocity - gloveBody.velocity;
				gloveBody.velocity += toVelocity * GLOVE_HOVER_SPEED * toPoint.GetMagnitude() / GLOVE_DIST_SPEED_BOOST * dt.asSeconds();
			}

			// Apply the component changes
			physicsManager_.SetBody(gloveEntity, gloveBody);
			SetComponent(gloveEntity, glove);
		}
	}
}

void game::GloveManager::StartReturn(const core::Entity gloveEntity)
{
	Glove glove = GetComponent(gloveEntity);

	glove.isRecovering = true;
	glove.recoveryTime = GLOVE_RECOVERY_TIME;

	Circle col = physicsManager_.Getcol(gloveEntity);
	col.isTrigger = false;
	col.enabled = false;
	physicsManager_.SetCol(gloveEntity, col);

	Body body = physicsManager_.GetBody(gloveEntity);
	body.velocity = core::Vec2f::zero();
	physicsManager_.SetBody(gloveEntity, body);

	glove.returningFromPos = body.position;

	SetComponent(gloveEntity, glove);
}

void game::GloveManager::StartPunch(const core::Entity gloveEntity)
{
	Glove glove = GetComponent(gloveEntity);

	glove.isPunching = true;
	glove.punchingTime = PUNCH_WINDUP_TIME;

	// Switch collider to trigger
	auto col = physicsManager_.Getcol(gloveEntity);
	col.isTrigger = true;

	physicsManager_.SetCol(gloveEntity, col);
	SetComponent(gloveEntity, glove);
}

void game::GloveManager::StartIdle(core::Entity gloveEntity)
{
	Glove glove = GetComponent(gloveEntity);

	glove.isPunching = false;
	glove.hasLaunched = false;
	glove.isRecovering = false;

	auto col = physicsManager_.Getcol(gloveEntity);
	col.isTrigger = false;
	col.enabled = true;
	physicsManager_.SetCol(gloveEntity, col);

	SetComponent(gloveEntity, glove);
}