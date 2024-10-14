//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <cstdint>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Scheduling {
    class Job;
    enum class JobKind : std::uint8_t;
}

namespace RbxStu::Scheduling {
    class TaskScheduler abstract {
    protected:
        std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > m_jobList;

    public:
        virtual ~TaskScheduler() = default;

        virtual bool ShouldStep(RbxStu::Scheduling::JobKind type, void *job, RBX::TaskScheduler::Job::Stats *jobStats);

        virtual void Step(RbxStu::Scheduling::JobKind jobType, void *robloxJob, RBX::TaskScheduler::Job::Stats *jobStats);
    };
} // RbxStu::Scheduling
