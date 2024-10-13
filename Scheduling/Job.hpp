//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include "TaskScheduler.hpp"

namespace RbxStu::Scheduling {
    class Job abstract {
    public:
        void Execute(RbxStu::Scheduling::TaskScheduler &scheduler);
    };
} // RbxStu::Scheduling
