#include <network/simulation_server.h>
#include <network/simulation_client.h>
#include <imgui.h>
#include <maths/basic.h>
#include <utils/conversion.h>
#include <utils/log.h>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif

namespace game
{
SimulationServer::SimulationServer(std::array<std::unique_ptr<SimulationClient>, 2>& clients) : clients_(clients)
{
}

void SimulationServer::Begin()
{

}

void SimulationServer::Update(sf::Time dt)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    auto packetIt = receivedPackets_.begin();
    while (packetIt != receivedPackets_.end())
    {
        packetIt->currentTime -= dt.asSeconds();
        if (packetIt->currentTime <= 0.0f)
        {
            ProcessReceivePacket(std::move(packetIt->packet));

            packetIt = receivedPackets_.erase(packetIt);
        }
        else
        {
            ++packetIt;
        }

    }

    packetIt = sentPackets_.begin();
    while (packetIt != sentPackets_.end())
    {
        packetIt->currentTime -= dt.asSeconds();
        if (packetIt->currentTime <= 0.0f)
        {
            for (auto& client : clients_)
            {
                client->ReceivePacket(packetIt->packet.get());
            }
            packetIt->packet = nullptr;
            packetIt = sentPackets_.erase(packetIt);
        }
        else
        {
            ++packetIt;
        }
    }
}

void SimulationServer::End()
{
}

void SimulationServer::DrawImGui()
{
    ImGui::Begin("Server");
    float minDelay = avgDelay_ - marginDelay_;
    float maxDelay = avgDelay_ + marginDelay_;
    bool hasDelayChanged = false;
    hasDelayChanged = hasDelayChanged || ImGui::SliderFloat("Min Delay", &minDelay, 0.01f, maxDelay);
    hasDelayChanged = hasDelayChanged || ImGui::SliderFloat("Max Delay", &maxDelay, minDelay, 1.0f);
    if (hasDelayChanged)
    {
        avgDelay_ = (maxDelay + minDelay) / 2.0f;
        marginDelay_ = (maxDelay - minDelay) / 2.0f;
    }
    ImGui::SliderFloat("Packet Loss", &packetLoss_, 0.0f, 1.0f);
    ImGui::End();
}

void SimulationServer::PutPacketInSendingQueue(std::unique_ptr<Packet> packet)
{
    sentPackets_.push_back({ avgDelay_ + core::RandomRange(-marginDelay_, marginDelay_), std::move(packet) });
}

void SimulationServer::PutPacketInReceiveQueue(std::unique_ptr<Packet> packet, bool unreliable)
{
    if(unreliable)
    {
        //Packet loss implementation
        if(core::RandomRange(0.0f, 1.0f) < packetLoss_)
        {
            return;
        }
    }
    receivedPackets_.push_back({ avgDelay_ + core::RandomRange(-marginDelay_, marginDelay_), std::move(packet) });
}

void SimulationServer::SendReliablePacket(std::unique_ptr<Packet> packet)
{
    PutPacketInSendingQueue(std::move(packet));
}

void SimulationServer::SendUnreliablePacket(std::unique_ptr<Packet> packet)
{
    PutPacketInSendingQueue(std::move(packet));
}

void SimulationServer::ProcessReceivePacket(std::unique_ptr<Packet> packet)
{
    Server::ReceivePacket(std::move(packet));
}

void SimulationServer::SpawnNewPlayer(ClientId clientId, PlayerNumber playerNumber)
{
    core::LogDebug("[Server] Spawn new player");
    auto spawnPlayer = std::make_unique<SpawnPlayerPacket>();
    spawnPlayer->packetType = PacketType::SPAWN_PLAYER;
    spawnPlayer->clientId = core::ConvertToBinary(clientId);
    spawnPlayer->playerNumber = playerNumber;

    const auto pos = SPAWN_POSITIONS[playerNumber] * 3.0f;
    spawnPlayer->pos = ConvertToBinary(pos);
    const auto rotation = SPAWN_ROTATIONS[playerNumber];
    spawnPlayer->angle = core::ConvertToBinary(rotation);
    gameManager_.SpawnPlayer(playerNumber, pos, rotation);
    gameManager_.SpawnGloves(playerNumber, pos, rotation);
    SendReliablePacket(std::move(spawnPlayer));
}
}
