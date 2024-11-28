//
// Created by Dottik on 28/10/2024.
//

#pragma once

#include <chrono>
#include <map>

#include "Scheduling/Job.hpp"

namespace RBX {
    enum DataModelType : std::int32_t;
}

namespace RbxStu::Scheduling::Jobs {
    class DataModelWatcherJob final : public RbxStu::Scheduling::Job {
        std::map<RBX::DataModelType, void *> m_dataModelPointerMap;
        std::map<RBX::DataModelType, std::chrono::steady_clock::time_point> m_dataModelLastTimeStepped;
        std::chrono::steady_clock::time_point m_lastStep;
        std::chrono::steady_clock::time_point m_lastCheck;

    public:
        DataModelWatcherJob() {
            this->m_lastCheck = std::chrono::high_resolution_clock::now();
        }

        ~DataModelWatcherJob() override = default;

        bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                        RBX::TaskScheduler::Job::Stats *jobStats) override;

        Jobs::AvailableJobs GetJobIdentifier() override;;

        void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                  RbxStu::Scheduling::TaskScheduler *scheduler) override;
    };
}
