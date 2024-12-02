//
// Created by Dottik on 2/12/2024.
//

#pragma once
#include <memory>

#include "Miscellaneous/OpaqueClass.hpp"
#include "World.hpp"

namespace RbxStu::Roblox {
    class World;
}
namespace RbxStu::Roblox {
    class Primitive final : public RbxStu::Miscellaneous::OpaqueClass {
    public:
        explicit Primitive(void *pObject) : OpaqueClass(pObject) {};

        static std::shared_ptr<RbxStu::Roblox::Primitive> FromBasePart(void *basePart) {
            return std::make_shared<RbxStu::Roblox::Primitive>(*(void **) ((std::uintptr_t) basePart + 0x160));
        }

        [[nodiscard]] std::shared_ptr<RbxStu::Roblox::World> GetWorld() const {
            // Offset Update: find 'new overlap in different world'
            // The offset mentioned in the if statement and below of the string is the world offset from an
            // RBX::Primitive object.
            return std::make_shared<RbxStu::Roblox::World>(
                    *reinterpret_cast<void **>(reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x1D0));
        }
    };

} // namespace RbxStu::Roblox
