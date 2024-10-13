//
// Created by Dottik on 12/10/2024.
//

#include "Scanner.hpp"

#include <mutex>

std::shared_ptr<RbxStu::Analysis::AOBScanner> RbxStu::Analysis::AOBScanner::pInstance;

bool RbxStu::Analysis::AOBScanner::IsInitialized() {
    return this->m_bIsInitialized;
}

void RbxStu::Analysis::AOBScanner::Initialize() {
    if (this->IsInitialized()) return;

    this->m_bIsInitialized = true;
}

std::mutex RbxStuAnalysisScannerGetSingletonMutex;

std::shared_ptr<RbxStu::Analysis::AOBScanner> RbxStu::Analysis::AOBScanner::GetSingleton() {
    if (RbxStu::Analysis::AOBScanner::pInstance == nullptr)
        RbxStu::Analysis::AOBScanner::pInstance = std::make_shared<RbxStu::Analysis::AOBScanner>();

    if (!RbxStu::Analysis::AOBScanner::pInstance->IsInitialized()) {
        std::scoped_lock lock{RbxStuAnalysisScannerGetSingletonMutex};
        if (RbxStu::Analysis::AOBScanner::pInstance->IsInitialized())
            return RbxStu::Analysis::AOBScanner::pInstance;

        RbxStu::Analysis::AOBScanner::pInstance->Initialize();
    }

    return RbxStu::Analysis::AOBScanner::pInstance;
}
