#include "game/effects.h"

#include "game/game_manager.h"

game::EffectManager::EffectManager(core::EntityManager& entityManager, GameManager& gameManager) :
	ComponentManager(entityManager), gameManager_(gameManager)

{
}

void game::EffectManager::Update(sf::Time dt)
{
#ifdef TRACY_ENABLE
	ZoneScoped;
#endif
	for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
	{
		if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
		{
			continue;
		}
		if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::EFFECT)))
		{
			auto& effect = GetComponent(entity);
			effect.lifetime -= dt.asSeconds();

			SetComponent(entity, effect);
			if (effect.lifetime < 0.0f)
			{
				gameManager_.DestroyEffect(entity);
			}
		}
	}
}