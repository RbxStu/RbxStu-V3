//
// Created by Dottik on 15/10/2024.
//

#pragma once

#include <vector>
#include <memory>

#include "Job.hpp"


namespace RbxStu::Concepts {
    template<typename Derived, typename Base>
    concept TypeConstraint = std::is_base_of_v<Base, Derived>;
}

namespace RBX {
    struct Time {
        double sec;
    };

    namespace TaskScheduler::Job {
        struct Stats {
            RBX::Time timeSinceStartup;
            RBX::Time deltaTime;
            RBX::Time previousDeltaTime;
        };
    }
}

namespace RbxStu::Scheduling {
    /*
     *  The type of job to modify and hook into (Execute at time)
     */
    enum class JobKind : std::uint8_t {
        Heartbeat,

        PhysicsJob,
        PhysicsStepJob,

        WaitingHybridScriptsJob,

        ModelMeshJob,

        PathUpdateJob,
        NavigationJob,

        Generic_UnknownJob, // Implemented by None Marshalled and some Replicator jobs!

        RenderJob,
        HttpRbxApi,

        LuauGarbageCollection,

        DebuggerConnection
    };

    class Job;
    enum class JobKind : std::uint8_t;
}

namespace RbxStu::Scheduling {
    class __declspec(dllimport, novtable) TaskScheduler final {
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
