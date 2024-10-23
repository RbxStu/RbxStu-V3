//
// Created by Dottik on 22/10/2024.
//

#pragma once
#include "Scheduling/Job/ExecuteScriptJob.hpp"

namespace RbxStu::StuLuau {
    class ExecutionEngine final {
        std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> m_executionEngineState;
    public:
        ExecutionEngine(
            std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> parentJobInitializationInformation);

        std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> GetInitializationInformation();
    };
} // RbxStu::Luau
