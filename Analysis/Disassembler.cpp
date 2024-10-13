//
// Created by Dottik on 12/10/2024.
//

#include "Disassembler.hpp"

#include <mutex>

std::shared_ptr<RbxStu::Analysis::Disassembler> RbxStu::Analysis::Disassembler::pInstance;

std::mutex RbxStuDisassemblerSingleton;

void RbxStu::Analysis::Disassembler::Initialize() {
    if (this->IsInitialized()) return; // Already initialized

    this->m_bIsInitialized = true;
}

bool RbxStu::Analysis::Disassembler::IsInitialized() {
    return this->m_bIsInitialized;
}

std::shared_ptr<RbxStu::Analysis::Disassembler> RbxStu::Analysis::Disassembler::GetSingleton() {
    if (RbxStu::Analysis::Disassembler::pInstance == nullptr)
        RbxStu::Analysis::Disassembler::pInstance = std::make_shared<RbxStu::Analysis::Disassembler>();

    if (!RbxStu::Analysis::Disassembler::pInstance->IsInitialized()) {
        std::scoped_lock lock{RbxStuDisassemblerSingleton};
        if (RbxStu::Analysis::Disassembler::pInstance->IsInitialized())
            return RbxStu::Analysis::Disassembler::pInstance;

        RbxStu::Analysis::Disassembler::pInstance->Initialize();
    }

    return RbxStu::Analysis::Disassembler::pInstance;
}
