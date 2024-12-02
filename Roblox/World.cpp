//
// Created by Dottik on 2/12/2024.
//

#include "World.hpp"

#include "Primitive.hpp"

namespace RbxStu::Roblox {
    std::shared_ptr<RbxStu::Roblox::World> World::FromPrimitive(std::shared_ptr<RbxStu::Roblox::Primitive> pPrimitive) {
        return pPrimitive->GetWorld();
    }
} // namespace RbxStu::Roblox
