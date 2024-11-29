//
// Created by Dottik on 29/11/2024.
//

#pragma once
#include <string>
#include "Miscellaneous/OpaqueClass.hpp"

namespace RbxStu::Roblox {

    class Script final : public RbxStu::Miscellaneous::OpaqueClass {
    public:
        explicit Script(void *pNative) : OpaqueClass(pNative) {}

        std::string GetScriptHash() const;
        std::string GetSource() const;
        std::string GetBytecode() const;
    };

} // namespace RbxStu::Roblox
