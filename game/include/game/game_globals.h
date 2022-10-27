/**
 * \file game_globals.h
 */

#pragma once
#include <array>

#include "engine/component.h"
#include "engine/entity.h"
#include "graphics/color.h"
#include "maths/angle.h"
#include "maths/vec2.h"


namespace game
{
/**
 * \brief PlayerNumber is a type used to define the number of the player.
 * Starting from 0 to maxPlayerNmb
 */
using PlayerNumber = std::uint8_t;
/**
 * \brief INVALID_PLAYER is an integer constant that defines an invalid player number.
 */
constexpr auto INVALID_PLAYER = std::numeric_limits<PlayerNumber>::max();
/**
 * \brief ClientId is a type used to define the client identification.
 * It is given by the server to clients.
 */
enum class ClientId : std::uint16_t {};
constexpr auto INVALID_CLIENT_ID = ClientId{ 0 };
using Frame = std::uint32_t;
/**
 * \brief mmaxPlayerNmb is a integer constant that defines the maximum number of player per game
 */
constexpr std::uint32_t maxPlayerNmb = 2;
constexpr float playerSpeed = 10.5f;
constexpr float playerMaxSpeed = 4.5f;
constexpr float playerFrictionLoss = 3.5f;
constexpr core::Degree playerRotationalSpeed(150.0f);
constexpr float playerColRadius = 0.5f;
constexpr float playerInvincibilityPeriod = 0.5f;
constexpr float invincibilityFlashPeriod = 0.1f;
constexpr float playerKnockbackTime = 0.5f;
constexpr float playerKnockbackScaling = 7.0f;
constexpr float playerBaseKnockbackMod = 2.0f;
constexpr float gloveKnockbackMod = 7.0f;

constexpr float gloveMinDist = 1.2f;
constexpr float gloveMaxDist = 1.6f;
constexpr float gloveIdealDist = 1.3f;
constexpr float gloveDamage = 30.0f;
constexpr float gloveColRadius = 0.4f;
/**
 * \brief Starting angle for the section where the glove is allowed to be in
 */
constexpr core::Degree gloveAngle1(20.0f);
/**
* \brief Ending angle for the section where the glove is allowed to be in
*/
constexpr core::Degree gloveAngle2(130.0f);
constexpr core::Degree gloveIdealAngle(40.0f);

constexpr float punchWindUptime = 0.05f;
constexpr float punchingTime = 0.18f;
constexpr float gloveRecoveryTime = 0.35f;
constexpr float punchingSpeed = 10.5f;
constexpr float gloveHoverSpeed = 1.0f;
constexpr float gloveDistSpeedBoost = 0.5f;

constexpr float battleStageHeight = 15.0f;
constexpr float battleStagewidth = 15.0f;
/**
 * \brief windowBufferSize is the size of input stored by a client. 5 seconds of frame at 50 fps
 */
constexpr std::size_t windowBufferSize = 5u * 50u;
/**
 * \brief startDelay is the delay to wait before starting a game in milliseconds
 */
constexpr long long startDelay = 3000;
/**
 * \brief maxInputNmb is the number of inputs stored into an PlayerInputPacket
 */
constexpr std::size_t maxInputNmb = 50;
/**
 * \brief fixedPeriod is the period used in seconds to start a new FixedUpdate method in the game::GameManager
 */
constexpr float fixedPeriod = 0.02f; //50fps

constexpr core::Color gloveOffColor(0,0,0, 155);
constexpr std::array<core::Color, std::max(4u, maxPlayerNmb)> playerColors
{
    core::Color::red(),
    core::Color::blue(),
    core::Color::yellow(),
    core::Color::cyan()
};

constexpr std::array<core::Vec2f, std::max(4u, maxPlayerNmb)> spawnPositions
{
    core::Vec2f(0,-1),
    core::Vec2f(0,1),
    core::Vec2f(-1,0),
    core::Vec2f(1,0),
};

constexpr std::array<core::Degree, std::max(4u, maxPlayerNmb)> spawnRotations
{
    core::Degree(0.0f),
    core::Degree(180.0f),
    core::Degree(-90.0f),
    core::Degree(90.0f)
};

enum class ComponentType : core::EntityMask
{
    PLAYER_CHARACTER = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE),
    GLOVE = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 1u,
    PLAYER_INPUT = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 2u,
    DESTROYED = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 3u,
};

/**
 * \brief PlayerInput is a type defining the input data from a player.
 */
using PlayerInput = std::uint8_t;

namespace PlayerInputEnum
{
enum PlayerInput : std::uint8_t
{
    NONE = 0u,
    UP = 1u << 0u,
    DOWN = 1u << 1u,
    LEFT = 1u << 2u,
    RIGHT = 1u << 3u,
    PUNCH = 1u << 4u,
    PUNCH2 = 1u << 5u
};
}
}
