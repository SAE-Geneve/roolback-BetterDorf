#include "game/game_manager.h"

#include "utils/log.h"

#include "maths/basic.h"
#include "utils/conversion.h"

#include <fmt/format.h>
#include <imgui.h>
#include <chrono>


#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif

namespace game
{

GameManager::GameManager() :
    transformManager_(entityManager_),
    rollbackManager_(*this, entityManager_)
{
    playerEntityMap_.fill(core::INVALID_ENTITY);
    gloveEntityMap_.fill(core::INVALID_ENTITY);
}

void GameManager::SpawnPlayer(PlayerNumber playerNumber, core::Vec2f position, core::Degree rotation)
{
    if (GetEntityFromPlayerNumber(playerNumber) != core::INVALID_ENTITY)
    {
        return;
    }
    core::LogDebug("[GameManager] Spawning new player");
    const auto entity = entityManager_.CreateEntity();
    playerEntityMap_[playerNumber] = entity;

    transformManager_.AddComponent(entity);
    transformManager_.SetPosition(entity, position);
    transformManager_.SetRotation(entity, rotation);
    rollbackManager_.SpawnPlayer(playerNumber, entity, position, rotation);
}

void GameManager::SpawnGloves(PlayerNumber playerNumber, core::Vec2f playerPos, core::Degree playerRot)
{
	const core::Entity playerEntity = GetEntityFromPlayerNumber(playerNumber);
    if (GetGlovesEntityFromPlayerNumber(playerNumber)[0] != core::INVALID_ENTITY)
    {
        return;
    }
    core::LogDebug("[GameManager] Spawning glove for player");
    // Create two gloves
    for (int gloveNum = 0; gloveNum < 2; gloveNum++)
    {
        const auto entity = entityManager_.CreateEntity();
        gloveEntityMap_[static_cast<long long>(playerNumber) * 2 + gloveNum] = entity;

        // Calculate position and rotation
        // Positions are swapped if glove is on the right
        const float sign = gloveNum == 0 ? 1.0f : -1.0f;
        const auto position = playerPos + core::Vec2f(0, GLOVE_IDEAL_DIST).Rotate(-playerRot).Rotate(GLOVE_IDEAL_ANGLE * sign);
        const auto rotation = playerRot;

        transformManager_.AddComponent(entity);
        transformManager_.SetPosition(entity, position);
        transformManager_.SetRotation(entity, rotation);

        // Inform the rest of the game about the glove's existence
        rollbackManager_.SpawnGlove(playerEntity, entity, position, rotation, sign);
    }

}

core::Entity GameManager::SpawnEffect(EffectType type, const core::Vec2f pos, float lifetime)
{
    return core::INVALID_ENTITY;
}

void GameManager::DestroyEffect(const core::Entity entity)
{
    rollbackManager_.DestroyEntity(entity);
}

core::Entity GameManager::GetEntityFromPlayerNumber(PlayerNumber playerNumber) const
{
    return playerEntityMap_[playerNumber];
}

std::array<core::Entity, 2> GameManager::GetGlovesEntityFromPlayerNumber(PlayerNumber playerNumber) const
{
    std::array<core::Entity, 2> gloves{};
    gloves[0] = gloveEntityMap_[static_cast<long long>(playerNumber) * 2];
    gloves[1] = gloveEntityMap_[static_cast<long long>(playerNumber) * 2 + 1];
    return gloves;
}

void GameManager::SetPlayerInput(PlayerNumber playerNumber, PlayerInput playerInput, std::uint32_t inputFrame)
{
    if (playerNumber == INVALID_PLAYER)
        return;

    rollbackManager_.SetPlayerInput(playerNumber, playerInput, inputFrame);

}
void GameManager::Validate(Frame newValidateFrame)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (rollbackManager_.GetCurrentFrame() < newValidateFrame)
    {
        rollbackManager_.StartNewFrame(newValidateFrame);
    }
    rollbackManager_.ValidateFrame(newValidateFrame);
}

PlayerNumber GameManager::CheckWinner()
{
    int winningPlayer = 0;
    PlayerNumber winner = INVALID_PLAYER;
    const auto& physicsManager = rollbackManager_.GetCurrentPhysicsManager();
    const auto& playerManager = rollbackManager_.GetPlayerCharacterManager();
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)))
            continue;
        const auto& playerBody = physicsManager.GetBody(entity);
        const auto& player = playerManager.GetComponent(entity);

        // Check if player is out of the bounds of the battleStage
        if (std::abs(playerBody.position.x) > BATTLE_STAGE_WIDTH / 2.0f ||
            std::abs(playerBody.position.y) > BATTLE_STAGE_HEIGHT / 2.0f)
        {
        }
        else
        {
            winningPlayer++;
            winner = player.playerNumber;
        }
    }

    if (winningPlayer == 0)
    {
        // If both loses, default to 
        return 1;
    }

    return winningPlayer == 1 ? winner : INVALID_PLAYER;
}

void GameManager::WinGame(PlayerNumber winner)
{
    winner_ = winner;
}

ClientGameManager::ClientGameManager(PacketSenderInterface& packetSenderInterface) :
    GameManager(),
    packetSenderInterface_(packetSenderInterface),
    spriteManager_(entityManager_, transformManager_),
    animationManager_(entityManager_, spriteManager_),
    effectManager_(entityManager_, *this)
{
}

void ClientGameManager::Begin()
{
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    //load textures
    if (!playerTexture_.loadFromFile("data/sprites/Eye.png"))
    {
        core::LogError("Could not load ship sprite");
    }
    if (!gloveTexture_.loadFromFile("data/sprites/Glove.png"))
    {
        core::LogError("Could not load glove's sprite");
    }
    //load fonts
    if (!font_.loadFromFile("data/fonts/8-bit-hud.ttf"))
    {
        core::LogError("Could not load font");
    }
    textRenderer_.setFont(font_);

    background_.Init(windowSize_);
    animationManager_.Init();
    soundPlayer_.Init();
}

void ClientGameManager::Update(const sf::Time dt)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (state_ & STARTED)
    {
        animationManager_.Update(dt);
        effectManager_.Update(dt);

        if (state_ & FINISHED)
        {
	        return;
        }

        rollbackManager_.SimulateToCurrentFrame();
        //Copy rollback transform position to our own
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (entityManager_.HasComponent(entity,
                static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER) |
                static_cast<core::EntityMask>(core::ComponentType::SPRITE)))
            {
                const auto& player = rollbackManager_.GetPlayerCharacterManager().GetComponent(entity);
                
                if (player.invincibilityTime > 0.0f &&
                    std::fmod(player.invincibilityTime, INVINCIBILITY_FLASH_PERIOD) > INVINCIBILITY_FLASH_PERIOD / 2.0f)
                {
                    spriteManager_.SetColor(entity, sf::Color::Black);
                }
                else
                {
                    spriteManager_.SetColor(entity, PLAYER_COLORS[player.playerNumber]);
                }
            }
        	else if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::GLOVE) |
				static_cast<core::EntityMask>(core::ComponentType::SPRITE)))
        	{
                const auto& glove = rollbackManager_.GetGloveManager().GetComponent(entity);

                if (glove.isRecovering)
                {
                    spriteManager_.SetColor(entity, GLOVE_OFF_COLOR);
                }
                else
                {
                    spriteManager_.SetColor(entity, PLAYER_COLORS[glove.playerNumber]);
                }
        	}

            if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::TRANSFORM)))
            {
                transformManager_.SetPosition(entity, rollbackManager_.GetTransformManager().GetPosition(entity));
                transformManager_.SetRotation(entity, rollbackManager_.GetTransformManager().GetRotation(entity));
            }
        }
    }

    fixedTimer_ += dt.asSeconds();
    while (fixedTimer_ > FIXED_PERIOD)
    {
        FixedUpdate();
        fixedTimer_ -= FIXED_PERIOD;
    }
}

void ClientGameManager::End()
{
}

void ClientGameManager::SetWindowSize(sf::Vector2u windowsSize)
{
    windowSize_ = windowsSize;
    const sf::FloatRect visibleArea(0.0f, 0.0f,
        static_cast<float>(windowSize_.x),
        static_cast<float>(windowSize_.y));
    originalView_ = sf::View(visibleArea);
    spriteManager_.SetWindowSize(sf::Vector2f(windowsSize));
    spriteManager_.SetCenter(sf::Vector2f(windowsSize) / 2.0f);
    auto& currentPhysicsManager = rollbackManager_.GetCurrentPhysicsManager();
    currentPhysicsManager.SetCenter(sf::Vector2f(windowsSize) / 2.0f);
    currentPhysicsManager.SetWindowSize(sf::Vector2f(windowsSize));

    background_.SetWindowSize(windowsSize);
}

void ClientGameManager::Draw(sf::RenderTarget& target)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    UpdateCameraView();
    target.setView(cameraView_);

    background_.Draw(target);
    spriteManager_.Draw(target);

    if(drawPhysics_)
    {
        auto& currentPhysicsManager = rollbackManager_.GetCurrentPhysicsManager();
        currentPhysicsManager.Draw(target);
    }

    // Draw texts on screen
    target.setView(originalView_);
    if (state_ & FINISHED)
    {
        if (winner_ == GetPlayerNumber())
        {
            const std::string winnerText = fmt::format("You won!");
            textRenderer_.setFillColor(sf::Color::White);
            textRenderer_.setString(winnerText);
            textRenderer_.setCharacterSize(32);
            const auto textBounds = textRenderer_.getLocalBounds();
            textRenderer_.setPosition(static_cast<float>(windowSize_.x) / 2.0f - textBounds.width / 2.0f,
                static_cast<float>(windowSize_.y) / 2.0f - textBounds.height / 2.0f);
            target.draw(textRenderer_);
        }
        else if (winner_ != INVALID_PLAYER)
        {
            const std::string winnerText = fmt::format("P{} won!", winner_ + 1);
            textRenderer_.setFillColor(sf::Color::White);
            textRenderer_.setString(winnerText);
            textRenderer_.setCharacterSize(32);
            const auto textBounds = textRenderer_.getLocalBounds();
            textRenderer_.setPosition(static_cast<float>(windowSize_.x) / 2.0f - textBounds.width / 2.0f,
                static_cast<float>(windowSize_.y) / 2.0f - textBounds.height / 2.0f);
            target.draw(textRenderer_);
        }
        else
        {
            const std::string errorMessage = fmt::format("Error with other players");
            textRenderer_.setFillColor(sf::Color::Red);
            textRenderer_.setString(errorMessage);
            textRenderer_.setCharacterSize(32);
            const auto textBounds = textRenderer_.getLocalBounds();
            textRenderer_.setPosition(static_cast<float>(windowSize_.x) / 2.0f - textBounds.width / 2.0f,
                static_cast<float>(windowSize_.y) / 2.0f - textBounds.height / 2.0f);
            target.draw(textRenderer_);
        }
    }
    if (!(state_ & STARTED))
    {
        if (startingTime_ != 0)
        {
            using namespace std::chrono;
            unsigned long long ms = duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()
                ).count();
            if (ms < startingTime_)
            {
                const std::string countDownText = fmt::format("Starts in {}", ((startingTime_ - ms) / 1000 + 1));
                textRenderer_.setFillColor(sf::Color::White);
                textRenderer_.setString(countDownText);
                textRenderer_.setCharacterSize(32);
                const auto textBounds = textRenderer_.getLocalBounds();
                textRenderer_.setPosition(static_cast<float>(windowSize_.x) / 2.0f - textBounds.width / 2.0f,
                    static_cast<float>(windowSize_.y) / 2.0f - textBounds.height / 2.0f);
                target.draw(textRenderer_);
            }
        }
    }
    else
    {
        std::string percent;
        const auto& playerManager = rollbackManager_.GetPlayerCharacterManager();
        for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
        {
            const auto playerEntity = GetEntityFromPlayerNumber(playerNumber);
            if (playerEntity == core::INVALID_ENTITY)
            {
                continue;
            }
            percent += fmt::format("P{}: {}%  ", playerNumber + 1, playerManager.GetComponent(playerEntity).damagePercent);
        }

        textRenderer_.setFillColor(sf::Color::White);
        textRenderer_.setString(percent);
        textRenderer_.setPosition(10, 10);
        textRenderer_.setCharacterSize(20);
        target.draw(textRenderer_);
    }

}

void ClientGameManager::SetClientPlayer(PlayerNumber clientPlayer)
{
    clientPlayer_ = clientPlayer;
}

void ClientGameManager::SpawnPlayer(PlayerNumber playerNumber, core::Vec2f position, core::Degree rotation)
{
    core::LogDebug(fmt::format("Spawn player: {}", playerNumber));

    GameManager::SpawnPlayer(playerNumber, position, rotation);
    const auto entity = GetEntityFromPlayerNumber(playerNumber);
    spriteManager_.AddComponent(entity);
    spriteManager_.SetTexture(entity, playerTexture_);
    spriteManager_.SetOrigin(entity, sf::Vector2f(playerTexture_.getSize()) / 2.0f);
    spriteManager_.SetColor(entity, PLAYER_COLORS[playerNumber]);
}

void ClientGameManager::SpawnGloves(PlayerNumber playerNumber, core::Vec2f playerPos, core::Degree playerRot)
{
	GameManager::SpawnGloves(playerNumber, playerPos,  playerRot);

    bool flip = true;
    for (const core::Entity& entity : GetGlovesEntityFromPlayerNumber(playerNumber))
    {
        spriteManager_.AddComponent(entity);
        spriteManager_.SetTexture(entity, gloveTexture_);
        spriteManager_.SetOrigin(entity, sf::Vector2f(gloveTexture_.getSize()) / 2.0f);
        spriteManager_.SetColor(entity, PLAYER_COLORS[playerNumber]);

        // Flip the glove if it's the first one
        if (flip)
        {
            transformManager_.SetScale(entity, { -1.0f, 1.0f });
        }

        flip = !flip;
    }
}

core::Entity ClientGameManager::SpawnEffect(const EffectType type, const core::Vec2f pos, float lifetime)
{
    const core::Entity entity = entityManager_.CreateEntity();

    rollbackManager_.SpawnEffect(entity, pos);

    transformManager_.AddComponent(entity);
    transformManager_.SetPosition(entity, pos);

    effectManager_.AddComponent(entity);
    auto effect = effectManager_.GetComponent(entity);
    effect.lifetime = lifetime;
    effect.type = type;
    effectManager_.SetComponent(entity, effect);

	switch (type)
	{
	case EffectType::HIT:
        animationManager_.SetupComponent(entity, animationManager_.hitEffect_);
        soundPlayer_.PlayEffectSound(SoundEffect::HIT);
		break;
	case EffectType::HIT_BIG:
        animationManager_.SetupComponent(entity, animationManager_.bigHitEffect_);
        soundPlayer_.PlayEffectSound(SoundEffect::HIT_BIG);
        break;
	case EffectType::SKULL:
		{
        animationManager_.SetupComponent(entity, animationManager_.growingSkull_);
        break;
		}
	case EffectType::TROPHY:
		{
        animationManager_.SetupComponent(entity, animationManager_.trophy_);
        break;
		}
    default: break;  // NOLINT(clang-diagnostic-covered-switch-default)
	}

    return entity;
}

void ClientGameManager::FixedUpdate()
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (!(state_ & STARTED))
    {
        if (startingTime_ != 0)
        {
            using namespace std::chrono;
            const auto ms = duration_cast<duration<unsigned long long, std::milli>>(
                system_clock::now().time_since_epoch()
                ).count();
            if (ms > startingTime_)
            {
                state_ = state_ | STARTED;
            }
            else
            {

                return;
            }
        }
        else
        {
            return;
        }
    }
    
    if (state_ & FINISHED)
    {
        return;
    }

    //We send the player inputs when the game started
    const auto playerNumber = GetPlayerNumber();
    if (playerNumber == INVALID_PLAYER)
    {
        //We still did not receive the spawn player packet, but receive the start game packet
        core::LogWarning(fmt::format("Invalid Player Entity in {}:line {}", __FILE__, __LINE__));
        return;
    }
    const auto& inputs = rollbackManager_.GetInputs(playerNumber);
    auto playerInputPacket = std::make_unique<PlayerInputPacket>();
    playerInputPacket->playerNumber = playerNumber;
    playerInputPacket->currentFrame = core::ConvertToBinary(currentFrame_);
    for (size_t i = 0; i < playerInputPacket->inputs.size(); i++)
    {
        if (i > currentFrame_)
        {
            break;
        }

        playerInputPacket->inputs[i] = inputs[i];
    }
    packetSenderInterface_.SendUnreliablePacket(std::move(playerInputPacket));

    currentFrame_++;
    rollbackManager_.StartNewFrame(currentFrame_);
}

void ClientGameManager::SetPlayerInput(PlayerNumber playerNumber, PlayerInput playerInput, std::uint32_t inputFrame)
{
    if (playerNumber == INVALID_PLAYER)
        return;
    GameManager::SetPlayerInput(playerNumber, playerInput, inputFrame);
}

void ClientGameManager::StartGame(unsigned long long int startingTime)
{
    core::LogDebug(fmt::format("Start game at starting time: {}", startingTime));
    startingTime_ = startingTime;
}

void ClientGameManager::DrawImGui()
{
    ImGui::Text(state_ & STARTED ? "Game has started" : "Game has not started");
    if (startingTime_ != 0)
    {
        ImGui::Text("Starting Time: %llu", startingTime_);
        using namespace std::chrono;
        unsigned long long ms = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
            ).count();
        ImGui::Text("Current Time: %llu", ms);
    }
    ImGui::Checkbox("Draw Physics", &drawPhysics_);
}

void ClientGameManager::ConfirmValidateFrame(Frame newValidateFrame,
    const std::array<PhysicsState, MAX_PLAYER_NMB>& physicsStates)
{
    if (newValidateFrame < rollbackManager_.GetLastValidateFrame())
    {
        core::LogWarning(fmt::format("New validate frame is too old"));
        return;
    }
    for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
    {
        if (rollbackManager_.GetLastReceivedFrame(playerNumber) < newValidateFrame)
        {
            
            core::LogWarning(fmt::format("Trying to validate frame {} while playerNumber {} is at input frame {}, client player {}",
                newValidateFrame,
                playerNumber + 1,
                rollbackManager_.GetLastReceivedFrame(playerNumber),
                GetPlayerNumber()+1));
            

            return;
        }
    }
    rollbackManager_.ConfirmFrame(newValidateFrame, physicsStates);
}

void ClientGameManager::WinGame(PlayerNumber winner)
{
    if (state_ & FINISHED)
        return;

    GameManager::WinGame(winner);

    const PlayerNumber loser = (winner + 1) % 2;
    const auto& loserBody = rollbackManager_.GetCurrentPhysicsManager().GetBody(GetEntityFromPlayerNumber(loser));
    const auto& winnerBody = rollbackManager_.GetCurrentPhysicsManager().GetBody(GetEntityFromPlayerNumber(winner));
    SpawnEffect(EffectType::SKULL, loserBody.position, END_EFFECTS_LIFETIME);
    SpawnEffect(EffectType::TROPHY, winnerBody.position, END_EFFECTS_LIFETIME);
    core::LogDebug("Winner declared on client");

    if (winner == GetPlayerNumber())
    {
        soundPlayer_.PlayEffectSound(SoundEffect::WIN);
    }
    else
    {
        soundPlayer_.PlayEffectSound(SoundEffect::LOSE);
    }

    state_ = state_ | FINISHED;
}

void ClientGameManager::UpdateCameraView()
{
    if ((state_ & STARTED) != STARTED)
    {
        cameraView_ = originalView_;
        return;
    }

    cameraView_ = originalView_;
    const sf::Vector2f extends{ cameraView_.getSize() / 2.0f / core::pixelPerMeter };
    float currentZoom = 1.0f;
    for (PlayerNumber playerNumber = 0; playerNumber < MAX_PLAYER_NMB; playerNumber++)
    {
        const auto playerEntity = GetEntityFromPlayerNumber(playerNumber);
        if (playerEntity == core::INVALID_ENTITY)
        {
            continue;
        }
        if (entityManager_.HasComponent(playerEntity, static_cast<core::EntityMask>(core::ComponentType::POSITION)))
        {
	        constexpr float margin = 1.0f;
	        const auto position = transformManager_.GetPosition(playerEntity);
            if (core::Abs(position.x) + margin > extends.x)
            {
                const auto ratio = (std::abs(position.x) + margin) / extends.x;
                if (ratio > currentZoom)
                {
                    currentZoom = ratio;
                }
            }
            if (core::Abs(position.y) + margin > extends.y)
            {
                const auto ratio = (std::abs(position.y) + margin) / extends.y;
                if (ratio > currentZoom)
                {
                    currentZoom = ratio;
                }
            }
        }
    }
    cameraView_.zoom(currentZoom);
}
}
