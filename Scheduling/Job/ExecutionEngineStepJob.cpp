//
// Created by Dottik on 27/10/2024.
//

#include "ExecutionEngineStepJob.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "InitializeExecutionEngineJob.hpp"
#include "Roblox/DataModel.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool ExecutionEngineStepJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                            RBX::TaskScheduler::Job::Stats *jobStats) {
        const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
        const auto execEngine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

        return jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob &&
               execEngine != nullptr && execEngine->GetInitializationInformation()->dataModel->GetRbxPointer() ==
               dataModel->GetRbxPointer();
    }

    Jobs::AvailableJobs ExecutionEngineStepJob::GetJobIdentifier() {
        return RbxStu::Scheduling::Jobs::AvailableJobs::SynchronizedDispatchJob;
    }

    void ExecutionEngineStepJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                      RbxStu::Scheduling::TaskScheduler *scheduler) {
        const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
        const auto currentExecutionEngine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

        if (nullptr == currentExecutionEngine)
            return; // ExecutionEngine not initialized for this DataModel


        if (const auto execEngineDataModel = currentExecutionEngine->GetInitializationInformation()->dataModel;
            execEngineDataModel->GetRbxPointer() != dataModel->GetRbxPointer())
            return;
        // DataModel is different, ExecutionEngine is out-of-date, reset required by InitializeExecutionEngineJob.


        /*
         *  Execution Step ordering:
         *      - Synchronized Dispatch
         *      - Execution
         *      - Yielding
         */
        currentExecutionEngine->StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep::SynchronizedDispatch);
        currentExecutionEngine->StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep::ExecuteStep);
        currentExecutionEngine->StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep::YieldStep);
    }
}
