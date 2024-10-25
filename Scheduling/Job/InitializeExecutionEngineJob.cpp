//
// Created by Dottik on 24/10/2024.
//

#include "InitializeExecutionEngineJob.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "lualib.h"
#include "Roblox/DataModel.hpp"
#include "Roblox/ScriptContext.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool InitializeExecutionEngineJob::ShouldStep(const RbxStu::Scheduling::JobKind jobKind, void *job,
                                                  RBX::TaskScheduler::Job::Stats *jobStats) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        const auto thisJob = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto engine = thisJob->GetExecutionEngine(dataModel->GetDataModelType());

        if (engine != nullptr) {
            const auto engineDataModel = engine->GetInitializationInformation()->dataModel;
            if (engineDataModel->GetDataModelType() == dataModel->GetDataModelType() && engineDataModel->GetRbxPointer()
                != dataModel->GetRbxPointer())
                return jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob; // DataModel has re-initialized.
        }

        return engine == nullptr && jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob;
    }

    Jobs::AvailableJobs InitializeExecutionEngineJob::GetJobIdentifier() {
        return Jobs::AvailableJobs::InitializeExecutionEngineJob;
    }

    void InitializeExecutionEngineJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                            RbxStu::Scheduling::TaskScheduler *scheduler) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto engine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

        /*
         *  We make a few assumptions here:
         *      - If we got to Step, that means that we MUST re-create the ExecutionEngine for the DataModelType this DataModelJob belongs to.
         *      - We are RBX::ScriptContextFacets::WaitingHybridScriptsJob.
         */

        // Assuming 'job' is WaitingHybridScriptsJob...
        auto scriptContext = RbxStu::Roblox::ScriptContext::FromWaitingHybridScriptsJob(job);

        const auto globalState = scriptContext->GetGlobalState();

        const auto rL = lua_newthread(globalState);
        lua_ref(globalState, -1);
        lua_pop(globalState, 1);
        const auto nL = lua_newthread(rL);
        lua_ref(rL, -1);
        lua_pop(rL, 1);

        luaL_sandboxthread(nL); // Sandbox to make renv != genv.

        const auto initData = std::make_shared<ExecutionEngineInitializationInformation>();
        initData->globalState = rL;
        initData->executorState = nL;
        initData->scriptContext = scriptContext;
        initData->dataModel = dataModel;

        auto didNotExistBefore = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType()).get() == nullptr;

        taskScheduler->CreateExecutionEngine(dataModel->GetDataModelType(), initData);

        if (didNotExistBefore) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                      std::format("Created RbxStu::StuLuau::ExecutionEngine for DataModel {}!", RBX::
                          DataModelTypeToString(
                              dataModel->GetDataModelType())));
        } else {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                      std::format("Re-Initialized RbxStu::StuLuau::ExecutionEngine for DataModel {} successfully!", RBX
                          ::
                          DataModelTypeToString(
                              dataModel->GetDataModelType())));
        }
    }
} // RbxStu::Scheduling::Jobs
