//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include "TaskScheduler.hpp"

namespace RbxStu::Scheduling {
    class Job abstract {
    public:
        virtual ~Job() = default;

        virtual bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                RBX::TaskScheduler::Job::Stats *jobStats);

        virtual void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                          RbxStu::Scheduling::TaskScheduler *scheduler);
    };
} // RbxStu::Scheduling
