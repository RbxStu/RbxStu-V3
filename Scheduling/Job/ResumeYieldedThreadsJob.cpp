//
// Created by Dottik on 14/10/2024.
//

#include "ResumeYieldedThreadsJob.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Roblox/DataModel.hpp"

namespace RbxStu::Scheduling {
    ResumeYieldedThreadsJob::~ResumeYieldedThreadsJob() = default;

    bool ResumeYieldedThreadsJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                             RBX::TaskScheduler::Job::Stats *jobStats) {
        // We can only resume threads when we are WaitingHybridScriptsJob
        if (jobKind != RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob) return false;

        const auto DataModel = RbxStu::Roblox::DataModel::FromJob(job);

        // There are threads waiting to be resumed.
        return !this->m_vResumptionList.at(DataModel->GetDataModelType()).empty();
    }

    void ResumeYieldedThreadsJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                       RbxStu::Scheduling::TaskScheduler *scheduler) {
        // TODO: Re-Implement yielding.
    }
} // RbxStu::Scheduling
