#include "game/animation_manager.h"

#include "game/game_manager.h"

namespace game
{
AnimationManager::AnimationManager(core::EntityManager& entityManager, core::SpriteManager& spriteManager) :
	ComponentManager(entityManager), spriteManager_(spriteManager)
{}

void AnimationManager::SetupComponent(core::Entity entity, Animation& animation)
{
	if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::ANIMATION_DATA)))
	{
		AddComponent(entity);
	}
	auto& data = GetComponent(entity);
	data.animation = &animation;
	SetComponent(entity, data);

	// Set the sprite's texture
	if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::SPRITE)))
	{
		spriteManager_.AddComponent(entity);
	}

	auto& sprite = spriteManager_.GetComponent(entity);
	sprite.setTexture(data.animation->animTexture);
	sprite.setOrigin(ANIMATION_PIXEL_SIZE / 2.0f, ANIMATION_PIXEL_SIZE / 2.0f);
	sprite.setTextureRect({ 0, 0,
		ANIMATION_PIXEL_SIZE, ANIMATION_PIXEL_SIZE });
	spriteManager_.SetComponent(entity, sprite);
}

void AnimationManager::Init()
{
	if (!hitEffect_.animTexture.loadFromFile("data/sprites/HitEffect.png"))
	{
		core::LogError("Failed to load hitEffect texture");
	}
	if (!bigHitEffect_.animTexture.loadFromFile("data/sprites/HitEffectBig.png"))
	{
		core::LogError("Failed to load bigHitEffect texture");
	}
}

void AnimationManager::Update(const sf::Time dt)
{
	for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
	{
		if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
		{
			continue;
		}

		if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::ANIMATION_DATA)))
		{
			continue;
		}

		auto data = GetComponent(entity);
		data.time += dt.asSeconds();
		const auto& [animTexture, looping] = *data.animation;

		auto sprite = spriteManager_.GetComponent(entity);
		if (data.time >= ANIMATION_PERIOD)
		{
			data.textureIdx++;
			if (data.textureIdx * ANIMATION_PIXEL_SIZE >= animTexture.getSize().x)
			{
				if (looping)
				{
					data.textureIdx = 0;
				}
				else
				{
					data.textureIdx = animTexture.getSize().x / ANIMATION_PIXEL_SIZE - ANIMATION_PIXEL_SIZE;
				}
			}
			data.time = 0.0f;

			sprite.setTextureRect({ data.textureIdx * ANIMATION_PIXEL_SIZE, 0,
				ANIMATION_PIXEL_SIZE, ANIMATION_PIXEL_SIZE });
			spriteManager_.SetComponent(entity, sprite);
		}

		SetComponent(entity, data);
	}
}
}