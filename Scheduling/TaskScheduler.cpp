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
        auto prev = this->m_executionEngines[dataModelType];
        prev.reset();
    }

    this->m_executionEngines[dataModelType] = std::make_shared<RbxStu::StuLuau::ExecutionEngine>(initInfo);
}

std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> RbxStu::Scheduling::TaskScheduler::GetExecutionEngine(
    const RBX::DataModelType dataModelType) const {
    return this->m_executionEngines.contains(dataModelType) ? this->m_executionEngines.at(dataModelType) : nullptr;
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
