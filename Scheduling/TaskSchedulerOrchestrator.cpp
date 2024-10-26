//
// Created by Dottik on 13/10/2024.
//

#include "TaskSchedulerOrchestrator.hpp"

#include <Logger.hpp>
#include <MinHook.h>
#include <Security.hpp>
#include <mutex>
#include <Analysis/Disassembler.hpp>
#include <libhat/Scanner.hpp>
#include <libhat/Signature.hpp>
#include <RTTIHook/VFTHook.h>

#include "Roblox/DataModel.hpp"

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

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_TaskSchedulerOrchestrator,
              "-- Creating RbxStu::Scheduling::TaskScheduler...");
    this->m_taskScheduler = std::make_shared<RbxStu::Scheduling::TaskScheduler>();

    std::map<std::string_view, RbxStu::Scheduling::JobKind> targets{
        {".?AVDebuggerConnectionJob@Studio@RBX@@", RbxStu::Scheduling::JobKind::DebuggerConnection},
        {".?AVModelMeshJob@RBX@@", RbxStu::Scheduling::JobKind::ModelMeshJob},
        {".?AVGcJob@ScriptContextFacets@RBX@@", RbxStu::Scheduling::JobKind::LuauGarbageCollection},
        {".?AVWaitingHybridScriptsJob@ScriptContextFacets@RBX@@", RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob},
        {".?AVHeartbeatTask@RBX@@", RbxStu::Scheduling::JobKind::Heartbeat},
        {".?AVPhysicsJob@RBX@@", RbxStu::Scheduling::JobKind::PhysicsJob},
        {".?AVRenderJob@Studio@RBX@@", RbxStu::Scheduling::JobKind::RenderJob},
        {".?AUPathUpdateJob@NavigationService@RBX@@", RbxStu::Scheduling::JobKind::PathUpdateJob},
        {".?AUNavigationJob@NavigationService@RBX@@", RbxStu::Scheduling::JobKind::NavigationJob},
        {".?AVGenericDataModelJob@RBX@@", RbxStu::Scheduling::JobKind::Generic_UnknownJob},
        {".?AVHttpRbxApiJob@RBX@@", RbxStu::Scheduling::JobKind::HttpRbxApi},
        {".?AVPhysicsStepJob@RBX@@", RbxStu::Scheduling::JobKind::PhysicsStepJob},
    };

    RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_TaskSchedulerOrchestrator,
              "-- Finding RTTI...");

    std::map<std::string_view, std::shared_ptr<RTTIScanner::RTTI> > classes{};

    std::vector<void *> pointerList{};
    for (const auto &[jobTypeDescriptorName, jobKind]: targets) {
        std::shared_ptr<RTTIScanner::RTTI> found = nullptr;
        auto demangled = RTTIScanner::RTTI::demangleName(jobTypeDescriptorName.data());
        r_RBX_DataModelJob_Step jobStepPointer = nullptr;
        for (const auto &[_, rtti]: RTTIScanner::classRTTI) {
            if (const auto name = rtti->pTypeDescriptor->name; strcmp(name, jobTypeDescriptorName.data()) == 0) {
                RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                          std::format("Found Step for {} @ {}", demangled, rtti->pVirtualFunctionTable [6]));
                found = rtti;
                jobStepPointer = static_cast<r_RBX_DataModelJob_Step>(rtti->pVirtualFunctionTable[6]);
                break;
            }
        }

        if (found == nullptr) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                      std::format("-- Failed to find VFT for {}", demangled));
            continue;
        }

        auto vftable = reinterpret_cast<RBX::DataModelJobVFTable *>(found->
            pVirtualFunctionTable);

        this->m_JobHooks[vftable] = std::make_shared<
            RbxStu::Scheduling::TaskSchedulerOrchestrator::DataModelJobStepHookMetadata>();

        this->m_JobHooks[vftable]->hookedJobName = demangled;
        this->m_JobHooks[vftable]->jobKind = jobKind;

        auto hookAttempt = MH_CreateHook(jobStepPointer,
                                         RbxStu::Scheduling::TaskSchedulerOrchestrator::__Hook__GenericJobStep,
                                         reinterpret_cast<void **>(&this->m_JobHooks[vftable]->original));
        if (this->m_JobHooks[vftable]->original != nullptr) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                      std::format("Established 0x{:x} --> 0x{:x}",
                          reinterpret_cast<uintptr_t>(RbxStu::Scheduling::TaskSchedulerOrchestrator::
                              __Hook__GenericJobStep),
                          reinterpret_cast<uintptr_t>(this->m_JobHooks[vftable]->original)));
            pointerList.push_back(jobStepPointer);
        } else if (hookAttempt == MH_STATUS::MH_ERROR_NOT_EXECUTABLE) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                      std::format("Memory is not executable --> 0x{:x}", reinterpret_cast<uintptr_t>(jobStepPointer)));
        }

        classes[demangled] = found;
    }

    for (const auto &pointer: pointerList)
        MH_EnableHook(pointer);

    this->m_bIsInitialized = true;
}

bool RbxStu::Scheduling::TaskSchedulerOrchestrator::__Hook__GenericJobStep(
    void **self, RBX::TaskScheduler::Job::Stats *timeMetrics) {
    const auto orchestrator = RbxStu::Scheduling::TaskSchedulerOrchestrator::pInstance;
    const auto jobOriginal = orchestrator->m_JobHooks[*reinterpret_cast<
        RBX::DataModelJobVFTable **>(self)]; // VFtable.

    if (!Roblox::DataModel::FromJob(self)->IsDataModelOpen()) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  "Refusing to step on a closed DataModel");
        return jobOriginal->original(self, timeMetrics);
    }

    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
              std::format("Stepping DataModel {} -- JobKind: {}", RBX::DataModelTypeToString(Roblox::DataModel::FromJob(self)->
                  GetDataModelType()), (int)jobOriginal->jobKind));

    orchestrator->GetTaskScheduler()->Step(jobOriginal->jobKind, self, timeMetrics);

    if (std::time(nullptr) - *RbxStu::Security::GetSingleton()->lastRan >= oxorany(15)) {
        while (oxorany(true) == oxorany(true)) {
            throw std::exception("RBXCRASH: whopsies");
        }
    }

    return jobOriginal->original(self, timeMetrics);
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

std::shared_ptr<RbxStu::Scheduling::TaskScheduler> RbxStu::Scheduling::TaskSchedulerOrchestrator::GetTaskScheduler() {
    return this->m_taskScheduler;
}
