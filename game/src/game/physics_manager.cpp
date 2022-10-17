#include "game/physics_manager.h"

#include <SFML/Graphics/CircleShape.hpp>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif

namespace game
{

PhysicsManager::PhysicsManager(core::EntityManager& entityManager) :
    entityManager_(entityManager), bodyManager_(entityManager), colManager_(entityManager)
{

}

bool radiiIntersect(const core::Vec2f pos1, const float r1, const core::Vec2f pos2, const float r2)
{
    return (pos1 - pos2).GetMagnitude() <= (r1 + r2);
}

void SolveOverlap(Body& rb1, Body& rb2, const float radii)
{
    //Find proportions of displacement according to masses, the less mass they have, the more they move
    const float m1 = rb1.mass;
    const float m2 = rb2.mass;
    float prop1 = m2 / (m1 + m2);
    float prop2 = m1 / (m1 + m2);

    if (rb1.bodyType == BodyType::STATIC)
    {
        if (rb2.bodyType == BodyType::STATIC)
        {
            prop2 = 0.0f;
            prop1 = 0.0f;
        }
        else
        {
            prop1 = 0.0f;
            prop2 = 1.0f;
        }
    }
    else if (rb2.bodyType == BodyType::STATIC) 
    {
        prop2 = 0.0f;
        prop1 = 1.0f;
    }

    static constexpr float epsilon = 0.01f;

    //Move shapes out of each other with minimum translation vector
    const core::Vec2f mtv = (rb1.position - rb2.position).GetNormalized() *
        (radii - (rb1.position - rb2.position).GetMagnitude() + epsilon);
    rb1.position = rb1.position + mtv * prop1;
    rb2.position = rb2.position - mtv * prop2;
}

void SolveVelocities(Body& rb1, Body& rb2)
{
    //handle static
    if (rb1.bodyType == BodyType::STATIC && rb2.bodyType == BodyType::STATIC)
        return;

    // Calculate normal
    const core::Vec2f normal = (rb1.position - rb2.position).GetNormalized();

    if (rb1.bodyType == BodyType::STATIC || rb2.bodyType == BodyType::STATIC)
    {
        Body& nonStatic = !(rb1.bodyType == BodyType::STATIC) ? rb1 : rb2;

        const core::Vec2f v = nonStatic.velocity;
        const core::Vec2f n = normal * -1.0;
        core::Vec2f v1;
        const float k = 2.0f * core::Vec2f::Dot(v, n);
        v1.x = v.x - k * n.x;
        v1.y = v.y - k * n.y;

        nonStatic.velocity = v1;
        return;
    }

    const float v1 = rb1.velocity.GetMagnitude();
	const float v2 = rb2.velocity.GetMagnitude();

    const float m1 = rb1.mass;
    const float m2 = rb2.mass;

    const float theta1 = std::atan2f(rb1.velocity.y, rb1.velocity.x);
    const float theta2 = std::atan2f(rb2.velocity.y, rb2.velocity.x);

    const float phi = std::atan2f(normal.y, normal.x);

    const float v1fx = ((v1 * std::cos(theta1 - phi) * (m1 - m2) + 2 * m2 * v2 * std::cos(theta2 - phi)) / (m1 + m2))
        * std::cos(phi) + v1 * std::sin(theta1 - phi) * std::cos(phi + core::PI / 2);
    const float v1fy = ((v1 * std::cos(theta1 - phi) * (m1 - m2) + 2 * m2 * v2 * std::cos(theta2 - phi)) / (m1 + m2))
        * std::sin(phi) + v1 * std::sin(theta1 - phi) * std::sin(phi + core::PI / 2);

    const float v2fx = ((v2 * std::cos(theta2 - phi) * (m2 - m1) + 2 * m1 * v1 * std::cos(theta1 - phi)) / (m2 + m1))
        * std::cos(phi) + v2 * std::sin(theta2 - phi) * std::cos(phi + core::PI / 2);
    const float v2fy = ((v2 * std::cos(theta2 - phi) * (m2 - m1) + 2 * m1 * v1 * std::cos(theta1 - phi)) / (m2 + m1))
        * std::sin(phi) + v2 * std::sin(theta2 - phi) * std::sin(phi + core::PI / 2);

    rb1.velocity = { v1fx, v1fy };
    rb2.velocity = { v2fx, v2fy };
}

void PhysicsManager::FixedUpdate(const sf::Time dt)
{
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    // Apply velocities
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
            continue;
        auto body = bodyManager_.GetComponent(entity);
        body.position += body.velocity * dt.asSeconds();
        body.rotation += body.angularVelocity * dt.asSeconds();
        bodyManager_.SetComponent(entity, body);
    }
    // Check collisions
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (!entityManager_.HasComponent(entity,
            static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
            static_cast<core::EntityMask>(core::ComponentType::CIRCLE_COLLIDER2D)) ||
            entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
            continue;
        for (core::Entity otherEntity = entity + 1; otherEntity < entityManager_.GetEntitiesSize(); otherEntity++)
        {
            if (!entityManager_.HasComponent(otherEntity,
                static_cast<core::EntityMask>(core::ComponentType::BODY2D) | static_cast<core::EntityMask>(core::ComponentType::CIRCLE_COLLIDER2D)) ||
                entityManager_.HasComponent(otherEntity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
                continue;
            Body& rb1 = bodyManager_.GetComponent(entity);
            const Circle& col1 = colManager_.GetComponent(entity);

        	Body& rb2 = bodyManager_.GetComponent(otherEntity);
            const Circle& col2 = colManager_.GetComponent(otherEntity);

            if (radiiIntersect(rb1.position, col1.radius, rb2.position, col2.radius))
            {
                if (col1.isTrigger || col2.isTrigger)
                {
					onTriggerAction_.Execute(entity, otherEntity);
                }
                else
                {
                    SolveVelocities(rb1, rb2);
                    SolveOverlap(rb1, rb2, col1.radius + col2.radius);
                }
            }
        }
    }
}
     
void PhysicsManager::SetBody(const core::Entity entity, const Body& body)
{
    bodyManager_.SetComponent(entity, body);
}

const Body& PhysicsManager::GetBody(const core::Entity entity) const
{
    return bodyManager_.GetComponent(entity);
}

void PhysicsManager::AddBody(const core::Entity entity)
{
    bodyManager_.AddComponent(entity);
}

void PhysicsManager::AddCol(const core::Entity entity)
{
    colManager_.AddComponent(entity);
}

void PhysicsManager::SetCol(const core::Entity entity, const Circle& col)
{
    colManager_.SetComponent(entity, col);
}

const Circle& PhysicsManager::Getcol(const core::Entity entity) const
{
    return colManager_.GetComponent(entity);
}

void PhysicsManager::RegisterTriggerListener(OnTriggerInterface& onTriggerInterface)
{
    onTriggerAction_.RegisterCallback(
        [&onTriggerInterface](const core::Entity entity1, const core::Entity entity2) { onTriggerInterface.OnTrigger(entity1, entity2); });
}

void PhysicsManager::CopyAllComponents(const PhysicsManager& physicsManager)
{
    bodyManager_.CopyAllComponents(physicsManager.bodyManager_.GetAllComponents());
    colManager_.CopyAllComponents(physicsManager.colManager_.GetAllComponents());
}

void PhysicsManager::Draw(sf::RenderTarget& renderTarget)
{
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (!entityManager_.HasComponent(entity,
            static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
            static_cast<core::EntityMask>(core::ComponentType::CIRCLE_COLLIDER2D)) ||
            entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
            continue;
        const auto& [radius, isTrigger] = colManager_.GetComponent(entity);
        const auto& body = bodyManager_.GetComponent(entity);
        sf::CircleShape circleShape;
        circleShape.setFillColor(core::Color::transparent());
        circleShape.setOutlineColor(core::Color::green());
        circleShape.setOutlineThickness(2.0f);
        const auto position = body.position;
        circleShape.setOrigin({ radius * core::pixelPerMeter, radius * core::pixelPerMeter });
        circleShape.setPosition(
            position.x * core::pixelPerMeter + center_.x,
            windowSize_.y - (position.y * core::pixelPerMeter + center_.y));
        circleShape.setRadius(radius * core::pixelPerMeter);
        renderTarget.draw(circleShape);
    }
}
}
