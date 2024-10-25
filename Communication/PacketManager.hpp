//
// Created by Pixeluted on 15/10/2024.
//

#pragma once
#include <memory>
#include <optional>

#include "PacketBase.hpp"

namespace RbxStu::Communication {
    class PacketManager {
    public:
        static std::map<PacketType, std::shared_ptr<PacketBase> > packetMap;

        static std::optional<std::string_view> HandleNewPacket(std::string_view jsonString);
    };
}
