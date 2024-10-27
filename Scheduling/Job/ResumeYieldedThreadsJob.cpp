//
// Created by Dottik on 14/10/2024.
//

#include "ResumeYieldedThreadsJob.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"

#include "Roblox/DataModel.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Scheduling::Jobs {
    ResumeYieldedThreadsJob::~ResumeYieldedThreadsJob() = default;

    bool ResumeYieldedThreadsJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                             RBX::TaskScheduler::Job::Stats *jobStats) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        if (!dataModel->IsDataModelOpen()) {
            const auto currentExecutionEngine = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                    GetExecutionEngine(
                        dataModel->GetDataModelType());

            if (nullptr != currentExecutionEngine)
                currentExecutionEngine->SetExecuteReady(false);
        }

        return jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob && !Roblox::DataModel::FromJob(job)->
               IsParallel(); // We do not want to resume in parallel, that is dangerous.
    }

    void ResumeYieldedThreadsJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                       RbxStu::Scheduling::TaskScheduler *scheduler) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        const auto currentExecutionEngine = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(
                    dataModel->GetDataModelType());

        if (nullptr == currentExecutionEngine)
            return; // ExecutionEngine not initialized for this DataModel


        if (const auto execEngineDataModel = currentExecutionEngine->GetInitializationInformation()->dataModel;
            execEngineDataModel->GetRbxPointer() != dataModel->GetRbxPointer() || !execEngineDataModel->
            IsDataModelOpen())
            return;
        // DataModel is different, ExecutionEngine is out-of-date, reset required by InitializeExecutionEngineJob.

        currentExecutionEngine->StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep::YieldStep);
    }
} // RbxStu::Scheduling
