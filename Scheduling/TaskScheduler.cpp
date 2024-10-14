//
// Created by Dottik on 12/10/2024.
//

#include "TaskScheduler.hpp"

#include <print>

#include "Job.hpp"
#include "Roblox/TypeDefinitions.hpp"

bool RbxStu::Scheduling::TaskScheduler::ShouldStep(RbxStu::Scheduling::JobKind type, void *job,
                                                   RBX::TaskScheduler::Job::Stats *jobStats) {
    /*
     *  This function must be overriden by inheritors, however we return true as we do not know what to expect.
     */
    return true;
}

void RbxStu::Scheduling::TaskScheduler::Step(const RbxStu::Scheduling::JobKind jobType, void *robloxJob,
                                             RBX::TaskScheduler::Job::Stats *jobStats) {
    if (!this->ShouldStep(jobType, robloxJob, jobStats)) return; // Just to be sure...

    for (const auto &job: this->m_jobList) {
        // Step all jobs that wish to run.
        if (job->ShouldStep(jobType, robloxJob, jobStats))
            job->Step(robloxJob, jobStats, this);
    }
}
