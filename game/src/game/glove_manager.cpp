#include "game/glove_manager.h"

game::GloveManager::GloveManager(core::EntityManager& entityManager, GameManager& gameManager) :
	ComponentManager(entityManager), gameManager_(gameManager)
{
}

void game::GloveManager::FixedUpdate(sf::Time dt)
{

}
