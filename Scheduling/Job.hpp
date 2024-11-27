//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include "TaskScheduler.hpp"

namespace RbxStu::Scheduling {
    namespace Jobs {
        enum class AvailableJobs : std::int32_t {
            GenericJob, // Not overriden.
            ImguiRenderJob,

            DataModelWatcherJob,
            SynchronizedDispatchJob,
            InitializeExecutionEngineJob,
            ExecuteScriptJob,
            ResumeYieldedThreadsJob,
        };
    }

    class Job abstract {
    public:
        virtual ~Job() = default;

        virtual void OnDestroy(RbxStu::Scheduling::JobKind jobKind, void *job) { return; };

        /*
         *  This function must be overridden by the one who inherits from this class.
         *      - True -> Execute Step function
         *      - False -> Does not execute Step function.
         */
        virtual bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                RBX::TaskScheduler::Job::Stats *jobStats) { return false; }

        virtual Jobs::AvailableJobs GetJobIdentifier() { return Jobs::AvailableJobs::GenericJob; }

        virtual void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                          RbxStu::Scheduling::TaskScheduler *scheduler) { return; }
    };
} // RbxStu::Scheduling
