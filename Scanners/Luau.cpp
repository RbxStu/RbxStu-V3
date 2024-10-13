//
// Created by Dottik on 13/10/2024.
//

#include "Luau.hpp"

#include "Analysis/Disassembler.hpp"

std::shared_ptr<RbxStu::Scanners::Luau> RbxStu::Scanners::Luau::pInstance;

std::shared_ptr<RbxStu::Scanners::Luau> RbxStu::Scanners::Luau::GetSingleton() {
    if (nullptr == RbxStu::Scanners::Luau::pInstance)
        RbxStu::Scanners::Luau::pInstance = std::make_shared<RbxStu::Scanners::Luau>();

    if (RbxStu::Scanners::Luau::pInstance->IsInitialized()) {
        std
    }
    return RbxStu::Scanners::Luau::pInstance;
}
