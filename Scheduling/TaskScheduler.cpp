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

std::recursive_mutex executionEngineMutex{};

void RbxStu::Scheduling::TaskScheduler::CreateExecutionEngine(const RBX::DataModelType dataModelType,
                                                              const std::shared_ptr<
                                                                  ExecutionEngineInitializationInformation> &
                                                              initInfo) {
    std::scoped_lock lg{executionEngineMutex};
    if (this->m_executionEngines.contains(dataModelType)) {
        // If it is the first load, then it will be false.
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Swapping Execution Engine for DataModel {}...", RBX::DataModelTypeToString(dataModelType)
                  ));
        auto prev = this->m_executionEngines[dataModelType];
        auto newObj = std::make_shared<RbxStu::StuLuau::ExecutionEngine>(initInfo);
        this->m_executionEngines[dataModelType].swap(newObj);
        prev->DestroyEngine();
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
                  std::format("Execution Engine for DataModel {} Swapped.", RBX::DataModelTypeToString(dataModelType)));
    }

    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
              std::format("Created ExecutionEngine for DataModel {} internally.", RBX::DataModelTypeToString(
                  dataModelType)));

    this->m_executionEngines[dataModelType] = std::make_shared<RbxStu::StuLuau::ExecutionEngine>(initInfo);
}

void RbxStu::Scheduling::TaskScheduler::ResetExecutionEngine(const RBX::DataModelType dataModelType) {
    std::scoped_lock lg{executionEngineMutex};
    if (this->m_executionEngines.contains(dataModelType)) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskScheduler,
                  std::format("ExecutionEngine for DataModel {} has been reset to nullptr.", RBX::DataModelTypeToString(
                      dataModelType)));
        auto execEngine = this->m_executionEngines.at(dataModelType);
        execEngine->DestroyEngine();
        this->m_executionEngines.erase(dataModelType);
    }
}

std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> RbxStu::Scheduling::TaskScheduler::GetExecutionEngine(
    const RBX::DataModelType dataModelType) {
    std::scoped_lock lg{executionEngineMutex};
    if (!this->m_executionEngines.contains(dataModelType)) {
        return nullptr;
    }

    auto currentEngine = this->m_executionEngines.at(dataModelType);

    return currentEngine;
}

std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> RbxStu::Scheduling::TaskScheduler::GetExecutionEngine(
    lua_State *L) const {
    std::scoped_lock lg{executionEngineMutex};
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
    auto dataModel = Roblox::DataModel::FromJob(robloxJob);

    // RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous,
    //           std::format("Job for {} is running! DataModel: {} & IsOpen: {}", RBX::DataModelTypeToString(dataModel->
    //               GetDataModelType
    //               ()), (void*)dataModel->GetRbxPointer(), dataModel->IsDataModelOpen()));

    for (const auto &job: this->m_jobList) {
        // Step all jobs that wish to run.
        if (job->ShouldStep(jobType, robloxJob, jobStats))
            job->Step(robloxJob, jobStats, this);
    }
}
