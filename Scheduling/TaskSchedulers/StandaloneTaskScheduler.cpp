//
// Created by Dottik on 14/10/2024.
//

#include "StandaloneTaskScheduler.hpp"

#include <Logger.hpp>

namespace RbxStu::Scheduling {
    StandaloneTaskScheduler::~StandaloneTaskScheduler() {
    }

    void StandaloneTaskScheduler::Step(const RbxStu::Scheduling::JobKind type, void *job,
                                       RBX::TaskScheduler::Job::Stats *jobStats) {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                  std::format("-- Standalone Task Scheduler Step: ", job));
        TaskScheduler::Step(type, job, jobStats);
    }
} // RbxStu::Scheduling
