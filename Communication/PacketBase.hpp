//
// Created by Pixeluted on 15/10/2024.
//
#pragma once
#include <Logger.hpp>
#include <cstdint>
#include <list>
#include <string_view>
#include <utility>
#include <nlohmann/json.hpp>


namespace RbxStu::Communication {
    enum PacketType : uint64_t {
        PacketBaseType, // Should never be used
        ScheduleExecutionType
    };

    class PacketBase abstract {
        uint64_t packetId{};
        uint64_t packetType{};
    public:
        virtual ~PacketBase() = default;

        /**
         * Every inheritor must also return the fields of the BasePacket
         * @brief Every packet must have this function otherwise we will throw an error
         * @return enum which determines fields of the packet required for validation
         */
        virtual std::list<std::string_view> Register() {
            return {"packetId", "packetType"};
        }

        /**
         * @brief This function should be used to validate all fields data type, the fields will have something, but here it is needed to be validated
         * @return a bool that determines if the packet action will be executed
         */
        virtual bool ValidateData(nlohmann::json jsonData) {
            return true;
        }

        /**
         * @brief This function will be executed whenever a valid packet of the type was sent
         */
        virtual void Callback(nlohmann::json jsonData) {
            RbxStuLog(RbxStu::LogType::Information, "RbxStuV3::PacketBase", "Base packet called!")
        }
    };
}
