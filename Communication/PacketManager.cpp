//
// Created by Pixeluted on 15/10/2024.
//

#include "PacketManager.hpp"
#include <nlohmann/json.hpp>

#include "Packets/ScheduleExecution.hpp"

namespace RbxStu::Communication {
    std::map<PacketType, std::shared_ptr<PacketBase>> PacketManager::packetMap = {
        { ScheduleExecutionType, std::make_shared<ScheduleExecution>() }
    };

    std::optional<std::string_view> PacketManager::HandleNewPacket(std::string_view jsonString) {
        nlohmann::json jsonData;
        try {
            jsonData = nlohmann::json::parse(jsonString);
        } catch (const nlohmann::json::parse_error& e) {
            return "Invalid JSON";
        }

        const auto baseTypeFields = { "packetId", "packetType" };
        for (const auto requiredFieldName : baseTypeFields) {
            if (!jsonData.contains(requiredFieldName)) {
                return std::format("Packet is missing field: {}", requiredFieldName);
            }
        }

        if (!jsonData["packetId"].is_number_integer() || !jsonData["packetType"].is_number_integer()) {
            return "Malformed packet";
        }

        uint64_t packetTypeValue = jsonData["packetType"].get<uint64_t>();
        const auto packetType = static_cast<PacketType>(packetTypeValue);

        if (!packetMap.contains(packetType)) {
            return "Invalid packet";
        }

        const auto& specificPacket = packetMap.at(packetType);
        const auto specificPacketFields = specificPacket->Register();
        for (const auto requiredFieldName : specificPacketFields) {
            if (!jsonData.contains(requiredFieldName)) {
                return std::format("Packet is missing field: {}", requiredFieldName);
            }
        }

        if (!specificPacket->ValidateData(jsonData)) {
            return "Malformed packet";
        }

        specificPacket->Callback(jsonData);
        return {};
    }
}
