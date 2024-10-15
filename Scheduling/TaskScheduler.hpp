//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <cstdint>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Scheduling {
    namespace Jobs {
        enum class AvailableJobs;
    }

    class Job;
    enum class JobKind : std::uint8_t;
}

namespace RbxStu::Scheduling {
    class TaskScheduler final {
        std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > m_jobList;

    public:
        template<Concepts::TypeConstraint<RbxStu::Scheduling::Job> T>
        __forceinline void AddSchedulerJob() {
            m_jobList.push_back(std::make_shared<T>());
        };

        [[nodiscard]] std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > GetJobs(
            RbxStu::Scheduling::Jobs::AvailableJobs jobIdentifier) const;

        void Step(RbxStu::Scheduling::JobKind jobType, void *robloxJob,
                  RBX::TaskScheduler::Job::Stats *jobStats);
    };
} // RbxStu::Scheduling
