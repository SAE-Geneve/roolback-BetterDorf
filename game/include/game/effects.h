#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
enum class EffectType
{
	HIT,
	HIT_BIG,
	SKULL,
	TROPHY
};

struct Effect
{
	float lifetime = EFFECTS_LIFETIME;
	EffectType type = EffectType::HIT;
};

class GameManager;

class EffectManager : public core::ComponentManager<Effect, static_cast<core::EntityMask>(ComponentType::EFFECT)>
{
public:
	explicit EffectManager(core::EntityManager&, GameManager&);
	void FixedUpdate(sf::Time dt);
private:
	GameManager& gameManager_;
};
}
