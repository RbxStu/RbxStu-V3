//
// Created by Dottik on 14/10/2024.
//

#pragma once

#include "Scheduling/TaskScheduler.hpp"

namespace RbxStu::Scheduling {
    class StandaloneTaskScheduler final : public RbxStu::Scheduling::TaskScheduler {
    public:
        ~StandaloneTaskScheduler() override;

        bool ShouldStep(RbxStu::Scheduling::JobKind type, void *job, RBX::TaskScheduler::Job::Stats *jobStats) override;

        void Step(RbxStu::Scheduling::JobKind type, void *job, RBX::TaskScheduler::Job::Stats *jobStats) override;
    };
} // RbxStu::Scheduling
