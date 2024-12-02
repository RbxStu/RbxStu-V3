//
// Created by Dottik on 2/12/2024.
//

#pragma once
#include <memory>

#include "Miscellaneous/OpaqueClass.hpp"

namespace RbxStu::Roblox {
    class Primitive;
    class World final : public RbxStu::Miscellaneous::OpaqueClass {
    public:
        explicit World(void *pObject) : RbxStu::Miscellaneous::OpaqueClass(pObject) {};

        static std::shared_ptr<RbxStu::Roblox::World>
        FromPrimitive(std::shared_ptr<RbxStu::Roblox::Primitive> pPrimitive);
    };
} // namespace RbxStu::Roblox
