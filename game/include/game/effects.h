#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
enum class EffectType
{
	HIT,
	HIT_BIG
};

struct Effect
{
	float lifetime = 1.0f;
	float startingTime = 1.0f;
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
