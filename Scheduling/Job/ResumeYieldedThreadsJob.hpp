//
// Created by Dottik on 14/10/2024.
//

#pragma once

#include <queue>

#include "Scheduling/Job.hpp"

namespace RbxStu::Scheduling::Jobs {
    struct ResumeThreadRequest {
        lua_State *pResumeThread;
        int lArgCount;
        int lReturnCount;

        bool bIsFulfilled;
    };

    class ResumeYieldedThreadsJob final : public Job {
        std::map<RBX::DataModelType, std::queue<RbxStu::Scheduling::Jobs::ResumeThreadRequest *> > m_vResumptionList;

    public:
        ~ResumeYieldedThreadsJob() override;

        bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                        RBX::TaskScheduler::Job::Stats *jobStats) override;

        void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                  RbxStu::Scheduling::TaskScheduler *scheduler) override;
    };
} // RbxStu::Scheduling
