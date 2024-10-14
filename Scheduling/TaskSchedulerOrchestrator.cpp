//
// Created by Dottik on 13/10/2024.
//

#include "TaskSchedulerOrchestrator.hpp"

#include <Logger.hpp>
#include <MinHook.h>
#include <mutex>
#include <Analysis/Disassembler.hpp>
#include <libhat/Scanner.hpp>
#include <libhat/Signature.hpp>
#include <RTTIHook/VFTHook.h>

std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator>
RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance;

void *originalWaitingHybridScriptsJob = nullptr;

bool waitingHybridScriptsJobStep(void *self) {
    printf("Hello, world! VFT hooked! %p\n", self);
    return reinterpret_cast<bool(*)(void *)>(originalWaitingHybridScriptsJob)(self);
}

void RbxStu::Scheduling::TaskSchedulerOrchestrator::Initialize() {
    if (this->m_bIsInitialized) return;

    MH_Initialize();

    std::vector<std::string_view> targets{
        ".?AVDebuggerConnectionJob@Studio@RBX@@",
        ".?AVModelMeshJob@RBX@@",
        ".?AVGcJob@ScriptContextFacets@RBX@@",
        ".?AVWaitingHybridScriptsJob@ScriptContextFacets@RBX@@",
        ".?AVHeartbeatTask@RBX@@",
        ".?AVPhysicsJob@RBX@@",
        ".?AVRenderJob@Studio@RBX@@",
        ".?AUPathUpdateJob@NavigationService@RBX@@",
        ".?AUNavigationJob@NavigationService@RBX@@",
        ".?AVScraperJob@TextScraper@RBX@@",
        ".?AVGenericDataModelJob@RBX@@",
        ".?AVGenericJob@DataModel@RBX@@",
        ".?AVHttpRbxApiJob@RBX@@",
        ".?AVPhysicsStepJob@RBX@@",
    };

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_TaskSchedulerOrchestrator,
              "-- Finding RTTI...");

    std::map<std::string_view, std::shared_ptr<RTTIScanner::RTTI> > classes{};

    for (const auto &rttiName: targets) {
        std::shared_ptr<RTTIScanner::RTTI> found = nullptr;
        auto demangled = RTTIScanner::RTTI::demangleName(rttiName.data());
        for (const auto &target: RTTIScanner::classRTTI) {
            auto name = target.second->pTypeDescriptor->name;

            if (strcmp(name, rttiName.data()) == 0) {
                RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                          std::format("Found Step for {} @ {}", demangled, target.second->pVirtualFunctionTable [6]));
                found = target.second;
                break;
            }
        }

        if (found == nullptr) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                      std::format("-- Failed to find VFT for {}", demangled));
            continue;
        }
        classes[demangled] = found;
    }

    this->m_bIsInitialized = true;
}


bool RbxStu::Scheduling::TaskSchedulerOrchestrator::IsInitialized() {
    return this->m_bIsInitialized;
}

std::mutex RbxStuSchedulingTaskSchedulerOrchestratorGetSingleton;

std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator>
RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton() {
    if (RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance == nullptr)
        RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance = std::make_shared<
            RbxStu::Scheduling::TaskSchedulerOrchestrator>();

    if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance->IsInitialized()) {
        std::scoped_lock lock{RbxStuSchedulingTaskSchedulerOrchestratorGetSingleton};
        if (RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance->IsInitialized())
            return
                    RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance;

        RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance->Initialize();
    }

    return RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance;
}
