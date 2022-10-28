#pragma once
#include <SFML/System/Time.hpp>
#include "SFML/Graphics/Texture.hpp"

#include "game_globals.h"


namespace core
{
class SpriteManager;
}

namespace game
{
class GameManager;

struct Animation
{
	sf::Texture animTexture;
    bool looping = false;
};

struct AnimationData
{
    float time = 0;
    int textureIdx = 0;
    Animation* animation;
};

/**
 * \brief AnimationManager is a ComponentManager that holds all the animations in one place.
 */
class AnimationManager final : public core::ComponentManager<AnimationData, static_cast<core::EntityMask>(ComponentType::ANIMATION_DATA)>
{
public:
    explicit AnimationManager(core::EntityManager& entityManager, core::SpriteManager& spriteManager);

    /**
     * \brief Add the component while also setting up it's animation and texture
     * \param entity The entity to add a component to
     * \param animation The animation to be played
     */
    void SetupComponent(core::Entity entity, Animation& animation);

    void Init();
    void Update(sf::Time dt);

    // TODO make this private and pass animations with enums
    Animation hitEffect_;
    Animation bigHitEffect_;

private:
    core::SpriteManager& spriteManager_;
};
}