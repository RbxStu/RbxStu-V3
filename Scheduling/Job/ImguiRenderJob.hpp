//
// Created by Dottik on 7/11/2024.
//

#pragma once
#include <Scheduling/Job.hpp>

namespace RbxStu::Scheduling::Jobs {
    class ImguiRenderJob final : public Job {
        std::atomic_bool m_bAreRenderHooksInitialized;

        void InitializeHooks();
    public:

        bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                        RBX::TaskScheduler::Job::Stats *jobStats) override;

        Jobs::AvailableJobs GetJobIdentifier() override { return Jobs::AvailableJobs::ImguiRenderJob; }

        void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                  RbxStu::Scheduling::TaskScheduler *scheduler) override;
    };
}
