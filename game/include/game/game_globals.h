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
constexpr float ANIMATION_PERIOD = 0.15f;
constexpr int ANIMATION_PIXEL_SIZE = 32;

constexpr std::uint32_t MAX_PLAYER_NMB = 2;
constexpr float PLAYER_SPEED = 10.5f;
constexpr float PLAYER_MAX_SPEED = 4.5f;
constexpr float PLAYER_FRICTION_LOSS = 3.5f;
constexpr core::Degree PLAYER_ROTATIONAL_SPEED(150.0f);
constexpr float PLAYER_COL_RADIUS = 0.5f;
constexpr float PLAYER_INVINCIBILITY_PERIOD = 0.5f;
constexpr float INVINCIBILITY_FLASH_PERIOD = 0.1f;
constexpr float PLAYER_KNOCKBACK_TIME = 0.5f;
constexpr float PLAYER_KNOCKBACK_SCALING = 7.0f;
constexpr float PLAYER_BASE_KNOCKBACK_MOD = 2.0f;
constexpr float GLOVE_KNOCKBACK_MOD = 7.0f;

constexpr float GLOVE_MIN_DIST = 1.2f;
constexpr float GLOVE_MAX_DIST = 1.6f;
constexpr float GLOVE_IDEAL_DIST = 1.3f;
constexpr float GLOVE_DAMAGE = 30.0f;
constexpr float GLOVE_COL_RADIUS = 0.4f;
/**
 * \brief Starting angle for the section where the glove is allowed to be in
 */
constexpr core::Degree GLOVE_ANGLE_1(20.0f);
/**
* \brief Ending angle for the section where the glove is allowed to be in
*/
constexpr core::Degree GLOVE_ANGLE_2(130.0f);
constexpr core::Degree GLOVE_IDEAL_ANGLE(40.0f);

constexpr float PUNCH_WINDUP_TIME = 0.05f;
constexpr float PUNCHING_TIME = 0.18f;
constexpr float GLOVE_RECOVERY_TIME = 0.85f;
constexpr float PUNCHING_SPEED = 10.5f;
constexpr float GLOVE_HOVER_SPEED = 1.5f;
constexpr float GLOVE_DIST_SPEED_BOOST = 0.5f;

constexpr float BATTLE_STAGE_HEIGHT = 15.0f;
constexpr float BATTLE_STAGE_WIDTH = 15.0f;
/**
 * \brief windowBufferSize is the size of input stored by a client. 5 seconds of frame at 50 fps
 */
constexpr std::size_t WINDOW_BUFFER_SIZE = 5u * 50u;
/**
 * \brief startDelay is the delay to wait before starting a game in milliseconds
 */
constexpr long long START_DELAY = 3000;
/**
 * \brief maxInputNmb is the number of inputs stored into an PlayerInputPacket
 */
constexpr std::size_t MAX_INPUT_NMB = 50;
/**
 * \brief fixedPeriod is the period used in seconds to start a new FixedUpdate method in the game::GameManager
 */
constexpr float FIXED_PERIOD = 0.02f; //50fps

constexpr core::Color GLOVE_OFF_COLOR(0,0,0, 155);
constexpr std::array<core::Color, std::max(4u, MAX_PLAYER_NMB)> PLAYER_COLORS
{
    core::Color::red(),
    core::Color::blue(),
    core::Color::yellow(),
    core::Color::cyan()
};

constexpr std::array<core::Vec2f, std::max(4u, MAX_PLAYER_NMB)> SPAWN_POSITIONS
{
    core::Vec2f(0,-1),
    core::Vec2f(0,1),
    core::Vec2f(-1,0),
    core::Vec2f(1,0),
};

constexpr std::array<core::Degree, std::max(4u, MAX_PLAYER_NMB)> SPAWN_ROTATIONS
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
    EFFECT = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 4u,
    ANIMATION_DATA = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 5u
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
