//
// Created by Dottik on 12/10/2024.
//

#include "TaskScheduler.hpp"

#include <Utilities.hpp>

#include "Job.hpp"
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
        const auto prev = this->m_executionEngines[dataModelType];
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

    if (L == nullptr) return nullptr;
    if (!Utilities::IsPointerValid(
        reinterpret_cast<void **>(reinterpret_cast<std::uintptr_t>(L) + offsetof(lua_State, global))))
        return nullptr;
    if (!Utilities::IsPointerValid(
        reinterpret_cast<void **>(*reinterpret_cast<std::uintptr_t **>(
            reinterpret_cast<std::uintptr_t>(L->global) + offsetof(
                global_State, mainthread)))))
        return nullptr;

    if (L->global == nullptr) return nullptr;

    // TODO: Change method of identifying the execution engine to use global_State instead of lua_State mainThread, more reliable.

    auto mainThread = lua_mainthread(L);

    for (const auto &engine: this->m_executionEngines | std::views::values) {
        if (engine->GetInitializationInformation() != nullptr && L->global != nullptr &&
            lua_mainthread(
                engine->GetInitializationInformation()->globalState) == mainThread)
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

void RbxStu::Scheduling::TaskScheduler::NotifyDestroy(RbxStu::Scheduling::JobKind jobType, void *robloxJob) {
    for (const auto &job: this->m_jobList) {
        job->OnDestroy(jobType, robloxJob);
    }
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
