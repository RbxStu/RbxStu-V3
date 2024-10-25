//
// Created by Dottik on 14/10/2024.
//

#include "ExecuteScriptJob.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "ldebug.h"
#include "luacode.h"
#include "lualib.h"
#include "Luau/BytecodeBuilder.h"
#include "Luau/Compiler.h"
#include "Luau/Compiler/src/BuiltinFolding.h"
#include "Roblox/DataModel.hpp"
#include "Roblox/ScriptContext.hpp"
#include "ixwebsocket/IXHttpServer.h"

#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Scheduling::Jobs {
    Jobs::AvailableJobs ExecuteScriptJob::GetJobIdentifier() {
        return AvailableJobs::ExecuteScriptJob;
    }

    ExecuteScriptJob::~ExecuteScriptJob() = default;

    void ExecuteScriptJob::ScheduleExecuteJob(RBX::DataModelType datamodelType, ExecuteJobRequest jobRequest) {
        std::lock_guard lock{executionQueueMutex};
        if (m_executionQueue.contains(datamodelType)) {
            m_executionQueue.at(datamodelType).push(jobRequest);
        }
    }

    bool ExecuteScriptJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                      RBX::TaskScheduler::Job::Stats *jobStats) {
        // This Job always executes as long as the Job is WaitingHybridScriptsJob.
        return jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob;
    }

    void ExecuteScriptJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                RbxStu::Scheduling::TaskScheduler *scheduler) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        const auto currentExecutionEngine = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(
                    dataModel->GetDataModelType());

        if (nullptr == currentExecutionEngine)
            return; // ExecutionEngine not initialized for this DataModel

        if (currentExecutionEngine->GetInitializationInformation()->dataModel->GetRbxPointer() != dataModel->GetRbxPointer())
            return; // DataModel is different, ExecutionEngine is out-of-date, reset required by InitializeExecutionEngineJob.

        currentExecutionEngine->StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep::ExecuteStep);
    }
}
