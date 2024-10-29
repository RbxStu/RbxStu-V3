//
// Created by Dottik on 27/10/2024.
//

#pragma once

#include "Scheduling/Job.hpp"

namespace RbxStu::Scheduling::Jobs {
    class SynchronizedDispatchJob final : public RbxStu::Scheduling::Job {
    public:
        ~SynchronizedDispatchJob() override = default;

        bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                        RBX::TaskScheduler::Job::Stats *jobStats) override;

        Jobs::AvailableJobs GetJobIdentifier() override;;

        void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                  RbxStu::Scheduling::TaskScheduler *scheduler) override;
    };
}
