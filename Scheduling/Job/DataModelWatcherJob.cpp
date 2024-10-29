//
// Created by Dottik on 28/10/2024.
//

#include "DataModelWatcherJob.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "InitializeExecutionEngineJob.hpp"
#include "Roblox/DataModel.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool DataModelWatcherJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                         RBX::TaskScheduler::Job::Stats *jobStats) {
        const auto dataModel = Roblox::DataModel::FromJob(job);
        this->m_dataModelPointerMap[dataModel->GetDataModelType()] = dataModel->GetRbxPointer();
        this->m_dataModelLastTimeStepped[dataModel->GetDataModelType()] = std::chrono::high_resolution_clock::now();
        const auto diff = std::chrono::high_resolution_clock::now() - this->m_lastCheck;

        return std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() > 16;
    }

    Jobs::AvailableJobs DataModelWatcherJob::GetJobIdentifier() {
        return Jobs::AvailableJobs::DataModelWatcherJob;
    }

    void DataModelWatcherJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                   RbxStu::Scheduling::TaskScheduler *scheduler) {
        this->m_lastCheck = std::chrono::high_resolution_clock::now();

        const auto taskScheduler = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        RBX::DataModelType dataModels[] = {
            RBX::DataModelType::DataModelType_MainMenuStandalone, RBX::DataModelType::DataModelType_Edit,
            RBX::DataModelType::DataModelType_PlayClient, RBX::DataModelType::DataModelType_PlayServer
        };

        for (const auto dataModel: dataModels) {
            auto execEngine = taskScheduler->GetExecutionEngine(dataModel);

            if (execEngine == nullptr) {
                continue;
            }
            if (m_dataModelPointerMap[dataModel] !=
                execEngine->GetInitializationInformation()->dataModel->GetRbxPointer()) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scheduling_Jobs_DataModelWatcherJob,
                          std::format("DataModel change detected for {} -- Resetting ExecutionEngine", RBX::
                              DataModelTypeToString(dataModel)));
                taskScheduler->ResetExecutionEngine(dataModel);
                continue;
            }

            auto compensationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - this->m_lastStep);

            auto lastSeen = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - this->m_dataModelLastTimeStepped[dataModel]);

            if (lastSeen.count() > compensationTime.count() + 1000) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Scheduling_Jobs_DataModelWatcherJob,
                          std::format(
                              "DataModel deletion detected for {}, too much time without stepping -- Resetting ExecutionEngine"
                              , RBX::
                              DataModelTypeToString(dataModel)));
                taskScheduler->ResetExecutionEngine(dataModel);
                continue;
            }
        }

        this->m_lastStep = std::chrono::high_resolution_clock::now();
    }
}
