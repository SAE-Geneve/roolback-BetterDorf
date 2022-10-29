#include <game/rollback_manager.h>
#include <game/game_manager.h>
#include "utils/assert.h"
#include <utils/log.h>
#include <fmt/format.h>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif

namespace game
{

RollbackManager::RollbackManager(GameManager& gameManager, core::EntityManager& entityManager) :
	gameManager_(gameManager), entityManager_(entityManager),
	currentTransformManager_(entityManager),
	currentPhysicsManager_(entityManager),
	currentPlayerManager_(entityManager, currentPhysicsManager_, gameManager_, currentGloveManager_), currentGloveManager_(entityManager, currentPhysicsManager_, gameManager),
	currentEffectManager_(entityManager, gameManager_),
	lastValidatedPhysicsManager_(entityManager),
	lastValidatedPlayerManager_(entityManager, lastValidatedPhysicsManager_, gameManager_, lastValidatedGloveManager_),
	lastValidatedGloveManager_(entityManager, lastValidatedPhysicsManager_, gameManager), lastValidatedEffectManager_(entityManager, gameManager_)
{
	for (auto& input : inputs_)
	{
		std::fill(input.begin(), input.end(), '\0');
	}
	currentPhysicsManager_.RegisterTriggerListener(*this);
}

void RollbackManager::SimulateToCurrentFrame()
{
#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	const auto currentFrame = gameManager_.GetCurrentFrame();
	const auto lastValidateFrame = gameManager_.GetLastValidateFrame();
	//Destroying all created Entities after the last validated frame
	for (const auto& createdEntity : createdEntities_)
	{
		if (createdEntity.createdFrame > lastValidateFrame)
		{
			entityManager_.DestroyEntity(createdEntity.entity);
		}
	}
	createdEntities_.clear();
	//Remove DESTROY flags
	for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
	{
		if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
		{
			entityManager_.RemoveComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
		}
	}

	//Revert the current game state to the last validated game state
	currentPhysicsManager_.CopyAllComponents(lastValidatedPhysicsManager_);
	currentPlayerManager_.CopyAllComponents(lastValidatedPlayerManager_.GetAllComponents());
	currentGloveManager_.CopyAllComponents(lastValidatedGloveManager_.GetAllComponents());
	currentEffectManager_.CopyAllComponents(lastValidatedEffectManager_.GetAllComponents());

	for (Frame frame = lastValidateFrame + 1; frame <= currentFrame; frame++)
	{
		testedFrame_ = frame;
		//Copy player inputs to player manager
		for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
		{
			const auto playerInput = GetInputAtFrame(playerNumber, frame);
			const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
			if (playerEntity == core::INVALID_ENTITY)
			{
				core::LogWarning(fmt::format("Invalid Entity in {}:line {}", __FILE__, __LINE__));
				continue;
			}
			PlayerCharacter playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
			playerCharacter.input = playerInput;
			currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
		}
		//Simulate one frame of the game
		currentPlayerManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
		currentGloveManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
		currentPhysicsManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
		currentEffectManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
	}
	//Copy the physics states to the transforms
	for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
	{
		if (!entityManager_.HasComponent(entity,
			static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
			static_cast<core::EntityMask>(core::ComponentType::TRANSFORM)))
			continue;
		const auto& body = currentPhysicsManager_.GetBody(entity);
		currentTransformManager_.SetPosition(entity, body.position);
		currentTransformManager_.SetRotation(entity, body.rotation);
	}
}

void RollbackManager::SetPlayerInput(PlayerNumber playerNumber, PlayerInput playerInput, Frame inputFrame)
{
	//Should only be called on the server
	if (currentFrame_ < inputFrame)
	{
		StartNewFrame(inputFrame);
	}
	inputs_[playerNumber][currentInputFrame_ - inputFrame] = playerInput;
	if (lastReceivedFrame_[playerNumber] < inputFrame)
	{
		lastReceivedFrame_[playerNumber] = inputFrame;
		//Repeat the same inputs until currentFrame
		for (size_t i = 0; i < currentInputFrame_ - inputFrame; i++)
		{
			inputs_[playerNumber][i] = playerInput;
		}
	}
}

void RollbackManager::StartNewFrame(Frame newFrame)
{

#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	if (currentInputFrame_ > newFrame)
		return;
	const auto delta = newFrame - currentInputFrame_;
	if (delta == 0)
	{
		return;
	}
	for (auto& inputs : inputs_)
	{
		for (auto i = inputs.size() - 1; i >= delta; i--)
		{
			inputs[i] = inputs[i - delta];
		}

		for (Frame i = 0; i < delta; i++)
		{
			inputs[i] = inputs[delta];
		}
	}
	currentInputFrame_ = newFrame;
}

void RollbackManager::ValidateFrame(Frame newValidateFrame)
{

#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	const auto lastValidateFrame = gameManager_.GetLastValidateFrame();
	//We check that we got all the inputs
	for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
	{
		if (GetLastReceivedFrame(playerNumber) < newValidateFrame)
		{
			gpr_assert(false, "We should not validate a frame if we did not receive all inputs!!!");
			return;
		}
	}
	//Destroying all created Entities after the last validated frame
	for (const auto& createdEntity : createdEntities_)
	{
		if (createdEntity.createdFrame > lastValidateFrame)
		{
			entityManager_.DestroyEntity(createdEntity.entity);
		}
	}
	createdEntities_.clear();
	//Remove DESTROYED flag
	for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
	{
		if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
		{
			entityManager_.RemoveComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
		}

	}
	createdEntities_.clear();

	//We use the current game state as the temporary new validate game state
	currentPhysicsManager_.CopyAllComponents(lastValidatedPhysicsManager_);
	currentPlayerManager_.CopyAllComponents(lastValidatedPlayerManager_.GetAllComponents());
	currentGloveManager_.CopyAllComponents(lastValidatedGloveManager_.GetAllComponents());
	currentEffectManager_.CopyAllComponents(lastValidatedEffectManager_.GetAllComponents());

	//We simulate the frames until the new validated frame
	for (Frame frame = lastValidatedFrame_ + 1; frame <= newValidateFrame; frame++)
	{
		testedFrame_ = frame;
		//Copy the players inputs into the player manager
		for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
		{
			const auto playerInput = GetInputAtFrame(playerNumber, frame);
			const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
			PlayerCharacter playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
			playerCharacter.input = playerInput;
			currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
		}
		//We simulate one frame
		currentPlayerManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
		currentGloveManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
		currentPhysicsManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
		currentEffectManager_.FixedUpdate(sf::seconds(FIXED_PERIOD));
	}
	//Definitely remove DESTROY entities
	for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
	{
		if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
		{
			entityManager_.DestroyEntity(entity);
		}
	}
	//Copy back the new validate game state to the last validated game state
	lastValidatedPlayerManager_.CopyAllComponents(currentPlayerManager_.GetAllComponents());
	lastValidatedGloveManager_.CopyAllComponents(currentGloveManager_.GetAllComponents());
	lastValidatedPhysicsManager_.CopyAllComponents(currentPhysicsManager_);
	lastValidatedEffectManager_.CopyAllComponents(currentEffectManager_.GetAllComponents());
	lastValidatedFrame_ = newValidateFrame;
	createdEntities_.clear();
}
void RollbackManager::ConfirmFrame(Frame newValidatedFrame, const std::array<PhysicsState, MAX_PLAYER_NMB>& serverPhysicsState)
{

#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	ValidateFrame(newValidatedFrame);
	for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
	{
		const PhysicsState lastPhysicsState = GetValidatePhysicsState(playerNumber);
		if (serverPhysicsState[playerNumber] != lastPhysicsState)
		{
			gpr_assert(false, fmt::format("Physics State are not equal for player {} (server frame: {}, client frame: {}, server: {}, client: {})",
				playerNumber + 1,
				newValidatedFrame,
				lastValidatedFrame_,
				serverPhysicsState[playerNumber],
				lastPhysicsState));
		}
	}
}
PhysicsState RollbackManager::GetValidatePhysicsState(PlayerNumber playerNumber) const
{
	PhysicsState state = 0;
	const core::Entity playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
	const std::array<core::Entity, 2> gloveEntities = gameManager_.GetGlovesEntityFromPlayerNumber(playerNumber);
	const auto& playerBody = lastValidatedPhysicsManager_.GetBody(playerEntity);
	const std::array<Body, 2>& gloveBodies = { lastValidatedPhysicsManager_.GetBody(gloveEntities[0]),
		lastValidatedPhysicsManager_.GetBody(gloveEntities[1]) };

	const auto* posPtr = reinterpret_cast<const PhysicsState*>(&playerBody.position);
	const auto* posPtr2 = reinterpret_cast<const PhysicsState*>(&gloveBodies[0].position);
	const auto* posPtr3 = reinterpret_cast<const PhysicsState*>(&gloveBodies[1].position);
	//Adding position
	for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
	{
		state += posPtr[i] + posPtr2[i] + posPtr3[i];
	}

	//Adding velocity
	const auto* velocityPtr = reinterpret_cast<const PhysicsState*>(&playerBody.velocity);
	const auto* velocityPtr2 = reinterpret_cast<const PhysicsState*>(&gloveBodies[0].velocity);
	const auto* velocityPtr3 = reinterpret_cast<const PhysicsState*>(&gloveBodies[1].velocity);
	for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
	{
		state += velocityPtr[i] + velocityPtr2[i] + velocityPtr3[i];
	}
	//Adding rotation
	const auto* anglePtr = reinterpret_cast<const PhysicsState*>(&playerBody.rotation);
	const auto* anglePtr2 = reinterpret_cast<const PhysicsState*>(&gloveBodies[0].rotation);
	const auto* anglePtr3 = reinterpret_cast<const PhysicsState*>(&gloveBodies[1].rotation);
	for (size_t i = 0; i < sizeof(float) / sizeof(PhysicsState); i++)
	{
		state += anglePtr[i] + anglePtr2[i] + anglePtr3[i];
	}
	//Adding angular Velocity
	const auto* angularVelPtr = reinterpret_cast<const PhysicsState*>(&playerBody.angularVelocity);
	const auto* angularVelPtr2 = reinterpret_cast<const PhysicsState*>(&gloveBodies[0].angularVelocity);
	const auto* angularVelPtr3 = reinterpret_cast<const PhysicsState*>(&gloveBodies[1].angularVelocity);
	for (size_t i = 0; i < sizeof(float) / sizeof(PhysicsState); i++)
	{
		state += angularVelPtr[i] + angularVelPtr2[i] + angularVelPtr3[i];
	}
	return state;
}

void RollbackManager::SpawnPlayer(PlayerNumber playerNumber, core::Entity entity, core::Vec2f position, core::Degree rotation)
{

#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	// Define player
	Body playerBody;
	playerBody.position = position;
	playerBody.rotation = rotation;
	constexpr Circle playerCol(PLAYER_COL_RADIUS);

	PlayerCharacter playerCharacter;
	playerCharacter.playerNumber = playerNumber;

	// Add and set components
	currentPlayerManager_.AddComponent(entity);
	currentPlayerManager_.SetComponent(entity, playerCharacter);

	currentPhysicsManager_.AddBody(entity);
	currentPhysicsManager_.SetBody(entity, playerBody);
	currentPhysicsManager_.AddCol(entity);
	currentPhysicsManager_.SetCol(entity, playerCol);

	lastValidatedPlayerManager_.AddComponent(entity);
	lastValidatedPlayerManager_.SetComponent(entity, playerCharacter);

	lastValidatedPhysicsManager_.AddBody(entity);
	lastValidatedPhysicsManager_.SetBody(entity, playerBody);
	lastValidatedPhysicsManager_.AddCol(entity);
	lastValidatedPhysicsManager_.SetCol(entity, playerCol);

	currentTransformManager_.AddComponent(entity);
	currentTransformManager_.SetPosition(entity, position);
	currentTransformManager_.SetRotation(entity, rotation);
}

void RollbackManager::SpawnGlove(core::Entity playerEntity, core::Entity entity, core::Vec2f position, core::Degree rotation, float sign)
{
#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	// Define the glove based on the player
	Body gloveBody;
	// Set the glove's position at its ideal location
	gloveBody.position = position;
	gloveBody.rotation = rotation;
	constexpr Circle gloveCol(GLOVE_COL_RADIUS);

	Glove glove;
	glove.sign = sign;
	glove.playerNumber = currentPlayerManager_.GetComponent(playerEntity).playerNumber;

	// Add and set components
	currentGloveManager_.AddComponent(entity);
	currentGloveManager_.SetComponent(entity, glove);

	currentPhysicsManager_.AddBody(entity);
	currentPhysicsManager_.SetBody(entity, gloveBody);
	currentPhysicsManager_.AddCol(entity);
	currentPhysicsManager_.SetCol(entity, gloveCol);

	lastValidatedGloveManager_.AddComponent(entity);
	lastValidatedGloveManager_.SetComponent(entity, glove);

	lastValidatedPhysicsManager_.AddBody(entity);
	lastValidatedPhysicsManager_.SetBody(entity, gloveBody);
	lastValidatedPhysicsManager_.AddCol(entity);
	lastValidatedPhysicsManager_.SetCol(entity, gloveCol);

	currentTransformManager_.AddComponent(entity);
	currentTransformManager_.SetPosition(entity, gloveBody.position);
	currentTransformManager_.SetRotation(entity, gloveBody.rotation);
}

void RollbackManager::SpawnEffect(core::Entity entity, EffectType type, core::Vec2f position)
{
	createdEntities_.push_back({ entity, testedFrame_ });

	currentEffectManager_.AddComponent(entity);
 	auto effect = currentEffectManager_.GetComponent(entity);
	effect.type = type;
	effect.lifetime = EFFECTS_LIFETIME;
	currentEffectManager_.SetComponent(entity, effect);

	currentTransformManager_.AddComponent(entity);
	currentTransformManager_.SetPosition(entity, position);
}

PlayerInput RollbackManager::GetInputAtFrame(PlayerNumber playerNumber, Frame frame) const
{
	gpr_assert(currentInputFrame_ - frame < inputs_[playerNumber].size(),
		"Trying to get input too far in the past");
	return inputs_[playerNumber][currentInputFrame_ - frame];
}

void RollbackManager::OnTrigger(core::Entity entity1, core::Entity entity2)
{
	if (entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
		entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::GLOVE)))
	{
		ManagePGCollision(entity1, entity2);

	}
	else if (entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
		entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::GLOVE)))
	{
		ManagePGCollision(entity2, entity1);
	}
	else if (entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::GLOVE)) &&
		entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::GLOVE)))
	{
		ManageGGCollision(entity1, entity2);
	}
}

void RollbackManager::DestroyEntity(core::Entity entity)
{

#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	//we don't need to save a bullet that has been created in the time window
	if (std::find_if(createdEntities_.begin(), createdEntities_.end(), [entity](auto newEntity)
		{
			return newEntity.entity == entity;
		}) != createdEntities_.end())
	{
		entityManager_.DestroyEntity(entity);
		return;
	}
		entityManager_.AddComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
}

void RollbackManager::ManagePGCollision(auto playerEntity, auto gloveEntity)
{
	PlayerCharacter player = currentPlayerManager_.GetComponent(playerEntity);
	const Glove glove = currentGloveManager_.GetComponent(gloveEntity);

	// Players can't hit themselves
	if (player.playerNumber == glove.playerNumber || player.invincibilityTime > 0.0f || !glove.hasLaunched)
	{
		return;
	}

	// Hit player char
	player.invincibilityTime = PLAYER_INVINCIBILITY_PERIOD;
	player.knockBackTime = PLAYER_KNOCKBACK_TIME;

	// Change glove properties
	currentGloveManager_.StartReturn(gloveEntity);

	const float knockbackMod = PLAYER_BASE_KNOCKBACK_MOD + PLAYER_KNOCKBACK_SCALING * player.damagePercent / 100.0f;
	player.damagePercent += GLOVE_DAMAGE;

	currentPlayerManager_.SetComponent(playerEntity, player);

	auto& gloveBody = currentPhysicsManager_.GetBody(gloveEntity);
	auto& playerBody = currentPhysicsManager_.GetBody(playerEntity);
	HandlePunchCollision(gloveBody, gloveEntity, playerBody, playerEntity, knockbackMod);

	gameManager_.SpawnEffect(EffectType::HIT_BIG, (gloveBody.position + playerBody.position) / 2.0f);
}

void RollbackManager::ManageGGCollision(auto firstGloveEntity, auto secondGloveEntity)
{
	const Glove glove1 = currentGloveManager_.GetComponent(firstGloveEntity);
	const Glove glove2 = currentGloveManager_.GetComponent(secondGloveEntity);

	Body glove1Body = currentPhysicsManager_.GetBody(firstGloveEntity);
	Body glove2Body = currentPhysicsManager_.GetBody(secondGloveEntity);

	const bool bothPunch = glove1.isPunching && glove2.isPunching && glove1.hasLaunched && glove2.hasLaunched;
	if (glove1.isPunching)
	{
		currentGloveManager_.StartReturn(firstGloveEntity);

		if (!bothPunch)
		{
			HandlePunchCollision(glove1Body, firstGloveEntity,
				glove2Body, secondGloveEntity, GLOVE_KNOCKBACK_MOD);
		}
	}
	if (glove2.isPunching)
	{
		currentGloveManager_.StartReturn(secondGloveEntity);

		if (!bothPunch)
		{
			HandlePunchCollision(glove2Body, secondGloveEntity,
				glove1Body, firstGloveEntity, GLOVE_KNOCKBACK_MOD);
		}
	}

	if (bothPunch)
	{
		// Zero out
		glove1Body.velocity = core::Vec2f::zero();
		glove2Body.velocity = core::Vec2f::zero();

		currentPhysicsManager_.SetBody(firstGloveEntity, glove1Body);
		currentPhysicsManager_.SetBody(secondGloveEntity, glove2Body);
	}

	gameManager_.SpawnEffect(EffectType::HIT, (glove1Body.position + glove2Body.position) / 2.0f);
}

void RollbackManager::HandlePunchCollision(Body gloveBody, core::Entity gloveEntity,
	Body otherBody, core::Entity otherEntity, float mod)
{
	otherBody.velocity = gloveBody.velocity.GetNormalized() * mod;

	gloveBody.velocity = core::Vec2f::zero();

	currentPhysicsManager_.SetBody(gloveEntity, gloveBody);
	currentPhysicsManager_.SetBody(otherEntity, otherBody);
}
}
