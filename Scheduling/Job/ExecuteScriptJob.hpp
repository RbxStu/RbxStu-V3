//
// Created by Dottik on 14/10/2024.
//

#pragma once

#include <queue>

#include "Scheduling/Job.hpp"

namespace RbxStu::Roblox {
    class DataModel;
    class ScriptContext;
}

namespace RbxStu::StuLuau {
    class ExecutionEngine;
}

namespace RbxStu::Scheduling {


    namespace Jobs {
        class ExecuteScriptJob final : public RbxStu::Scheduling::Job {
        public:
            ~ExecuteScriptJob() override;

            Jobs::AvailableJobs GetJobIdentifier() override;

            bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                            RBX::TaskScheduler::Job::Stats *jobStats) override;

            void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                      RbxStu::Scheduling::TaskScheduler *scheduler) override;
        };
    } // Jobs
} // RbxStu::Scheduling
