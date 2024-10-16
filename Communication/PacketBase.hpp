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
    public:
        virtual ~PacketBase() = default;

        /**
         * @brief Every packet must have this function otherwise we will throw an error
         * @return list of strings determining fields required for this packet
         */
        virtual std::list<std::string_view> GetRequiredFields() {
            return {"packetType"};
        }

        /**
         * @brief This function should be used to validate all fields data type, the fields will have something, but here it is needed to be validated
         * @return a bool that determines if the packet callback will be executed
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
