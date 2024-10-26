//
// Created by Dottik on 12/10/2024.
//

#include "TaskScheduler.hpp"
#include "Job.hpp"
#include "Job/ExecuteScriptJob.hpp"
#include "Job/InitializeExecutionEngineJob.hpp"
#include "Roblox/DataModel.hpp"
#include "Roblox/TypeDefinitions.hpp"
#include "StuLuau/ExecutionEngine.hpp"

void RbxStu::Scheduling::TaskScheduler::CreateExecutionEngine(const RBX::DataModelType dataModelType,
                                                              const std::shared_ptr<
                                                                  ExecutionEngineInitializationInformation> &
                                                              initInfo) {
    if (this->m_executionEngines.contains(dataModelType)) {
        // If it is the first load, then it will be false.
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Swapping Execution Engine for DataModel {}...", RBX::DataModelTypeToString(dataModelType)
                  ));
        auto prev = this->m_executionEngines[dataModelType];
        auto newObj = std::make_shared<RbxStu::StuLuau::ExecutionEngine>(initInfo);
        this->m_executionEngines[dataModelType].swap(newObj);
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
                  std::format("Execution Engine for DataModel {} Swapped.", RBX::DataModelTypeToString(dataModelType)));
    }

    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
              std::format("Created ExecutionEngine for DataModel {} internally.", RBX::DataModelTypeToString(
                  dataModelType)));

    this->m_executionEngines[dataModelType] = std::make_shared<RbxStu::StuLuau::ExecutionEngine>(initInfo);
}

void RbxStu::Scheduling::TaskScheduler::ResetExecutionEngine(const RBX::DataModelType dataModelType) {
    if (this->m_executionEngines.contains(dataModelType)) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
                  std::format("ExecutionEngine for DataModel {} has been reset to nullptr.", RBX::DataModelTypeToString(
                      dataModelType)));
        this->m_executionEngines.erase(dataModelType);
    }
}

std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> RbxStu::Scheduling::TaskScheduler::GetExecutionEngine(
    const RBX::DataModelType dataModelType)  {
    if (!this->m_executionEngines.contains(dataModelType)) {
        return nullptr;
    }

    auto currentEngine = this->m_executionEngines.at(dataModelType);

    if (currentEngine == nullptr || currentEngine->GetInitializationInformation() == nullptr || !currentEngine->
        GetInitializationInformation()->dataModel->IsDataModelOpen() || !currentEngine->
        GetInitializationInformation()->dataModel->CheckPointer()) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
                  std::format(
                      "DataModel is closed for Execution Engine {}, it's pointer is invalid or it has been swapped whilst requesting information. Resetting for next call"
                      , RBX::DataModelTypeToString(
                          dataModelType)));
        this->ResetExecutionEngine(dataModelType);
        return nullptr;
    }

    return currentEngine;
}

std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> RbxStu::Scheduling::TaskScheduler::GetExecutionEngine(
    lua_State *L) const {
    for (const auto &engine: this->m_executionEngines | std::views::values) {
        if (lua_mainthread(engine->GetInitializationInformation()->globalState) == lua_mainthread(L))
            return engine;
    }

    return nullptr;
}

std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > RbxStu::Scheduling::TaskScheduler::GetJobs(
    const RbxStu::Scheduling::Jobs::AvailableJobs jobIdentifier) const {
    std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > jobs{};
    for (const auto &job: this->m_jobList) {
        if (job->GetJobIdentifier() == jobIdentifier) {
            jobs.push_back(job);
        }
    }

    return jobs;
}

void RbxStu::Scheduling::TaskScheduler::Step(const RbxStu::Scheduling::JobKind jobType, void *robloxJob,
                                             RBX::TaskScheduler::Job::Stats *jobStats) {
    if (!Roblox::DataModel::FromJob(robloxJob)->IsDataModelOpen())
        return; // TaskScheduler should not step on DataModel's which have been closed.

    for (const auto &job: this->m_jobList) {
        // Step all jobs that wish to run.
        if (job->ShouldStep(jobType, robloxJob, jobStats)) {
            job->Step(robloxJob, jobStats, this);
        }
    }
}
