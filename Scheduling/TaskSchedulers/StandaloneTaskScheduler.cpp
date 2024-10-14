//
// Created by Dottik on 14/10/2024.
//

#include "StandaloneTaskScheduler.hpp"

#include <Logger.hpp>

namespace RbxStu::Scheduling {
    StandaloneTaskScheduler::~StandaloneTaskScheduler() = default;

    bool StandaloneTaskScheduler::ShouldStep(RbxStu::Scheduling::JobKind type, void *job,
                                             RBX::TaskScheduler::Job::Stats *jobStats) {
        /*
         *  We must read the DataModelType inside of the Job by obtaining the 'fake' DataModel and then the "Real" DataModel this job belongs to.
         *  After which we must read its property to determine whether or not we should run, if this function returns true, then TaskScheduler::Step will run!
         */

        return TaskScheduler::ShouldStep(type, job, jobStats);
    }

    void StandaloneTaskScheduler::Step(const RbxStu::Scheduling::JobKind type, void *job,
                                       RBX::TaskScheduler::Job::Stats *jobStats) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                  std::format("-- Standalone Task Scheduler Step: ", job));
        return TaskScheduler::Step(type, job, jobStats);
    }
} // RbxStu::Scheduling
