//
// Created by Dottik on 14/10/2024.
//

#pragma once

#include <queue>

#include "Scheduling/Job.hpp"

namespace RbxStu::Roblox {
    class DataModel;
    class ScriptContext;
}

namespace RbxStu::StuLuau {
    class ExecutionEngine;
}

namespace RbxStu::Scheduling {
    struct ExecuteJobRequest {
        bool bGenerateNativeCode;
        std::string_view scriptSource;
    };

    struct ExecuteScriptJobInitializationInformation {
        lua_State *globalState;
        lua_State *executorState;
        std::shared_ptr<RbxStu::Roblox::ScriptContext> scriptContext;
        std::shared_ptr<RbxStu::Roblox::DataModel> dataModel;
    };

    namespace Jobs {
        class ExecuteScriptJob final : public RbxStu::Scheduling::Job {
            std::map<RBX::DataModelType, std::shared_ptr<StuLuau::ExecutionEngine> > m_stateMap;

            std::mutex executionQueueMutex;
            std::map<RBX::DataModelType, std::queue<RbxStu::Scheduling::ExecuteJobRequest> > m_executionQueue;

            bool ShouldReinitialize(void *job);

            void InitializeForDataModel(void *job);

            std::optional<std::shared_ptr<StuLuau::ExecutionEngine> >
            GetInitializationContext(void *job);

        public:
            ~ExecuteScriptJob() override;

            void ScheduleExecuteJob(RBX::DataModelType datamodelType, ExecuteJobRequest jobRequest);

            Jobs::AvailableJobs GetJobIdentifier() override;

            bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                            RBX::TaskScheduler::Job::Stats *jobStats) override;


            void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                      RbxStu::Scheduling::TaskScheduler *scheduler) override;
        };
    } // Jobs
} // RbxStu::Scheduling
