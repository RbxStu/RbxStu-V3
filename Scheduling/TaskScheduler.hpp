//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <memory>
#include <map>

#include "Job.hpp"
#include "Roblox/TypeDefinitions.hpp"

#undef GetJob
namespace RbxStu::StuLuau {
    class ExecutionEngine;
}

namespace RbxStu::Scheduling {
    struct ExecutionEngineInitializationInformation;

    namespace Jobs {
        enum class AvailableJobs;
    }

    class Job;
    enum class JobKind : std::uint8_t;
}

namespace RbxStu::Scheduling {
    class TaskScheduler final {
        std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > m_jobList;
        std::map<RBX::DataModelType, std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> > m_executionEngines;

    public:
        void CreateExecutionEngine(RBX::DataModelType dataModelType,
                                   const std::shared_ptr<RbxStu::Scheduling::ExecutionEngineInitializationInformation> &
                                   initInfo);

        void ResetExecutionEngine(RBX::DataModelType dataModelType);

        [[nodiscard]] std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> GetExecutionEngine(
            RBX::DataModelType dataModelType);

        [[nodiscard]] std::shared_ptr<RbxStu::StuLuau::ExecutionEngine> GetExecutionEngine(lua_State *L) const;

        [[nodiscard]] bool IsDataModelActive(RBX::DataModelType dataModelType);;

        template<Concepts::TypeConstraint<RbxStu::Scheduling::Job> T>
        __forceinline void AddSchedulerJob() {
            m_jobList.push_back(std::make_shared<T>());
        };

        template<RbxStu::Concepts::TypeConstraint<RbxStu::Scheduling::Job> T>
        std::optional<std::shared_ptr<T> > GetSchedulerJob(const Jobs::AvailableJobs jobIdentifier) {
            for (const auto &job: this->m_jobList) {
                if (job->GetJobIdentifier() == jobIdentifier) {
                    return std::dynamic_pointer_cast<T>(job);
                }
            }

            return {};
        };

        [[nodiscard]] std::vector<std::shared_ptr<RbxStu::Scheduling::Job> > GetJobs(
            RbxStu::Scheduling::Jobs::AvailableJobs jobIdentifier) const;

        void NotifyDestroy(RbxStu::Scheduling::JobKind jobType, void *robloxJob);

        void Step(RbxStu::Scheduling::JobKind jobType, void *robloxJob,
                  RBX::TaskScheduler::Job::Stats *jobStats);
    };
} // RbxStu::Scheduling
