//
// Created by Dottik on 12/10/2024.
//

#include <RTTIHook/RTTIScanner.h>
#include "RTTI.hpp"

#include <Logger.hpp>
#include <mutex>
#include <optional>

std::shared_ptr<RbxStu::Analysis::RTTI> RbxStu::Analysis::RTTI::pInstance;

std::mutex RbxStuAnalysisRTTIGetSingleton;

bool RbxStu::Analysis::RTTI::IsInitialized() {
    return this->m_bIsInitialized;
}

void RbxStu::Analysis::RTTI::Initialize() {
    if (this->IsInitialized()) return;

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_RTTI, "-- Scanning for RTTI...");
    this->pRTTIScanner = std::make_shared<RTTIScanner>();

    this->pRTTIScanner->scan();
    this->pRTTIScanner->scan();
    this->pRTTIScanner->scan();
    this->pRTTIScanner->scan();
    this->pRTTIScanner->scan();

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_RTTI,
              std::format("-- RTTI scan completed, found {} RTTI objects in PE.", RTTIScanner::classRTTI.size()));

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Analysis_RTTI,
              "-- Results saved to memory.");


    this->m_bIsInitialized = true;
}

std::optional<std::shared_ptr<RTTIScanner::RTTI> > RbxStu::Analysis::RTTI::GetRuntimeTypeInformation(
    const std::string_view name) {
    if (!this->IsInitialized()) return {}; // Invalid access.

    if (!RTTIScanner::classRTTI.contains(name))
        return {};

    auto info = RTTIScanner::classRTTI.at(name);
    return info;
}

std::shared_ptr<RbxStu::Analysis::RTTI> RbxStu::Analysis::RTTI::GetSingleton() {
    if (RbxStu::Analysis::RTTI::pInstance == nullptr)
        RbxStu::Analysis::RTTI::pInstance = std::make_shared<RbxStu::Analysis::RTTI>();

    if (!RbxStu::Analysis::RTTI::pInstance->IsInitialized()) {
        std::scoped_lock lock{RbxStuAnalysisRTTIGetSingleton};
        if (RbxStu::Analysis::RTTI::pInstance->IsInitialized())
            return RbxStu::Analysis::RTTI::pInstance;

        RbxStu::Analysis::RTTI::pInstance->Initialize();
    }
    return RbxStu::Analysis::RTTI::pInstance;
}
