//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include "TaskScheduler.hpp"

namespace RbxStu::Scheduling {
    class Job abstract {
    public:
        virtual ~Job() = default;

        virtual void Execute(RbxStu::Scheduling::TaskScheduler &scheduler);

        virtual void ParallelExecute(RbxStu::Scheduling::TaskScheduler &scheduler);
    };
} // RbxStu::Scheduling
