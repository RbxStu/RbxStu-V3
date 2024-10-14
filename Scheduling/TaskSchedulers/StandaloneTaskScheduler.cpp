//
// Created by Dottik on 14/10/2024.
//

#include "StandaloneTaskScheduler.hpp"

#include <ios>
#include <Logger.hpp>
#include <sstream>

namespace RbxStu::Scheduling
{
    StandaloneTaskScheduler::~StandaloneTaskScheduler() = default;

    bool StandaloneTaskScheduler::ShouldStep(RbxStu::Scheduling::JobKind type, void* job,
                                             RBX::TaskScheduler::Job::Stats* jobStats)
    {
        /*
         *  We must read the DataModelType inside of the Job by obtaining the 'fake' DataModel and then the "Real" DataModel this job belongs to.
         *  After which we must read its property to determine whether or not we should run, if this function returns true, then TaskScheduler::Step will run!
         */

        /*
         * Jobs always have a pointer to a fake Datamodel and that fake data model has pointer to a real Datamodel
         * Offset Explanation:
         * Job + 0xB0 - Fake Datamodel
         * Fake Datamodel + 0x188 - Real Datamodel
         */
        auto* dataModel = reinterpret_cast<RBX::DataModel*>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(job) + 0xB0) + 0x188);

        std::stringstream ss;
        ss << "Datamodel pointer: 0x" << std::hex << reinterpret_cast<uintptr_t>(dataModel)
           << "\nDatamodel Type: " << dataModel->m_dwDataModelType;
        RbxStuLog(RbxStu::Debug, "RbxStu::StandaloneTaskScheduler", ss.str());


        return TaskScheduler::ShouldStep(type, job, jobStats);
    }

    void StandaloneTaskScheduler::Step(const RbxStu::Scheduling::JobKind type, void* job,
                                       RBX::TaskScheduler::Job::Stats* jobStats)
    {
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_TaskSchedulerOrchestrator,
                  std::format("-- Standalone Task Scheduler Step! Job: 0x{:x} --", reinterpret_cast<uintptr_t>(job)));
        return TaskScheduler::Step(type, job, jobStats);
    }
} // RbxStu::Scheduling
