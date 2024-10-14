//
// Created by Dottik on 12/10/2024.
//

#include "TaskScheduler.hpp"

#include <print>

#include "Job.hpp"
#include "Roblox/TypeDefinitions.hpp"

void RbxStu::Scheduling::TaskScheduler::Step(const RbxStu::Scheduling::JobKind jobType, void *robloxJob,
                                             RBX::TaskScheduler::Job::Stats *jobStats) {
    for (const auto &job: this->m_jobList) {
        // Step all jobs that wish to run.
        if (job->ShouldStep(jobType, robloxJob, jobStats)) {
            job->Step(robloxJob, jobStats, this);
        }
    }
}
