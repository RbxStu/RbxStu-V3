//
// Created by Dottik on 12/10/2024.
//

#pragma once
#include <cstdint>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Scheduling {
    enum class JobKind : std::uint8_t;
}

namespace RbxStu::Scheduling {
    class TaskScheduler abstract {
    public:
        virtual ~TaskScheduler() = default;

        virtual void Step(RbxStu::Scheduling::JobKind type, void *job, RBX::TaskScheduler::Job::Stats *jobStats);
    };
} // RbxStu::Scheduling
