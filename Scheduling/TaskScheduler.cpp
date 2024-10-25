//
// Created by Dottik on 12/10/2024.
//

#include "TaskScheduler.hpp"
#include "Job.hpp"
#include "Job/ExecuteScriptJob.hpp"
#include "Roblox/TypeDefinitions.hpp"
#include "StuLuau/ExecutionEngine.hpp"

void RbxStu::Scheduling::TaskScheduler::CreateExecutionEngine(RBX::DataModelType dataModelType,
                                                              std::shared_ptr<ExecutionEngineInitializationInformation>
                                                              initInfo) {
    auto prev = this->m_executionEngines[dataModelType];
    prev.reset();

    this->m_executionEngines[dataModelType] = std::make_shared<RbxStu::StuLuau::ExecutionEngine>(initInfo);
}

std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> RbxStu::Scheduling::TaskScheduler::GetExecutionEngine(
    const RBX::DataModelType dataModelType) {
    return this->m_executionEngines.at(dataModelType);
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
    for (const auto &job: this->m_jobList) {
        // Step all jobs that wish to run.
        if (job->ShouldStep(jobType, robloxJob, jobStats)) {
            job->Step(robloxJob, jobStats, this);
        }
    }
}
