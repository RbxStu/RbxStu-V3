//
// Created by Dottik on 22/10/2024.
//

#include "ExecutionEngine.hpp"

namespace RbxStu::StuLuau {
    ExecutionEngine::ExecutionEngine(
        std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> parentJobInitializationInformation) {
        this->m_executionEngineState = parentJobInitializationInformation;
    }

    std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> ExecutionEngine::
    GetInitializationInformation() {
        return this->m_executionEngineState;
    }
} // RbxStu::Luau
