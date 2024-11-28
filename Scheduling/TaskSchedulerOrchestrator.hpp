//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <atomic>
#include <map>
#include <memory>

#include "Job.hpp"
#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Scheduling {
    /*
     *  The type of job to modify and hook into (Execute at time)
     */
    enum class JobKind : std::uint8_t {
        Heartbeat,

        PhysicsJob,
        PhysicsStepJob,

        WaitingHybridScriptsJob,

        ModelMeshJob,

        PathUpdateJob,
        NavigationJob,

        Generic_UnknownJob, // Implemented by None Marshalled and some Replicator jobs!

        RenderJob,
        HttpRbxApi,

        LuauGarbageCollection,

        DebuggerConnection
    };

    class TaskSchedulerOrchestrator final {
        using r_RBX_DataModelJob_Step = bool(__fastcall *)(void *self, RBX::TaskScheduler::Job::Stats *deltaTime);

        struct DataModelJobStepHookMetadata {
            std::string hookedJobName;
            JobKind jobKind;
            r_RBX_DataModelJob_Step original;

            void (__fastcall*destroyJobOriginal)(void *);
        };

        static std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator> pInstance;

        std::shared_ptr<RbxStu::Scheduling::TaskScheduler> m_taskScheduler;

        std::map<std::string_view, RBX::DataModelJobVFTable **> jobVirtualFunctionTableMap;

        std::atomic_bool m_bIsInitialized;

        std::map<RBX::DataModelJobVFTable *,
            std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator::DataModelJobStepHookMetadata> > m_JobHooks;

        void Initialize();

        static void __Hook__DestroyGenericJob(void **self);

        static bool __Hook__GenericJobStep(void **self, RBX::TaskScheduler::Job::Stats *timeMetrics);

    public:
        bool IsInitialized();

        static std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator> GetSingleton();

        std::shared_ptr<RbxStu::Scheduling::TaskScheduler> GetTaskScheduler();;
    };
} // RbxStu::Scheduling
