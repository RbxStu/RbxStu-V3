//
// Created by Dottik on 12/10/2024.
//

#include "Scanner.hpp"

#include <mutex>

std::shared_ptr<RbxStu::Analysis::Scanner> RbxStu::Analysis::Scanner::pInstance;

bool RbxStu::Analysis::Scanner::IsInitialized() {
    return this->m_bIsInitialized;
}

void RbxStu::Analysis::Scanner::Initialize() {
    if (this->IsInitialized()) return;

    this->m_bIsInitialized = true;
}

std::mutex RbxStuAnalysisScannerGetSingletonMutex;

std::shared_ptr<RbxStu::Analysis::Scanner> RbxStu::Analysis::Scanner::GetSingleton() {
    if (RbxStu::Analysis::Scanner::pInstance == nullptr)
        RbxStu::Analysis::Scanner::pInstance = std::make_shared<RbxStu::Analysis::Scanner>();

    if (!RbxStu::Analysis::Scanner::pInstance->IsInitialized()) {
        std::scoped_lock lock{RbxStuAnalysisScannerGetSingletonMutex};
        if (RbxStu::Analysis::Scanner::pInstance->IsInitialized())
            return RbxStu::Analysis::Scanner::pInstance;

        RbxStu::Analysis::Scanner::pInstance->Initialize();
    }

    return RbxStu::Analysis::Scanner::pInstance;
}
