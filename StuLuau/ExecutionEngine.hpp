//
// Created by Dottik on 22/10/2024.
//

#pragma once
#include <functional>

#include "Scheduling/Job/ExecuteScriptJob.hpp"

namespace RbxStu::StuLuau {
    enum class ExecutionEngineStep : std::int32_t {
        YieldStep,
        ExecuteStep,
    };

    enum class ExecutionSecurity {
        LocalScript, // Level 3
        Plugin, // Level 7
        RobloxExecutor // Level 8
    };

    struct YieldResult {
        bool bIsSuccess{};
        std::int32_t dwNumberOfReturns{};
        std::optional<std::string> szErrorMessage;
    };

    struct YieldRequest {
        bool bIsReady;  // should be an std::atomic_bool, but thanks to the fun bits of fucking C++ STL it doesn't WORK, thank you for deleting the ONLY constructor I need.
        lua_State *lpResumeTarget;
        RBX::Lua::WeakThreadRef threadRef;
        std::function<YieldResult()> fpCompletionCallback;
    };

    struct ExecuteRequest {
        std::string szLuauCode;
        RbxStu::StuLuau::ExecutionSecurity executeWithSecurity;
    };

    class ExecutionEngine final {
        std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> m_executionEngineState;

        std::queue<std::shared_ptr<RbxStu::StuLuau::YieldRequest> > m_yieldQueue;
        std::queue<RbxStu::StuLuau::ExecuteRequest> m_executeQueue;

    public:
        explicit ExecutionEngine(
            std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> parentJobInitializationInformation);

        std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> GetInitializationInformation();

        void StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep stepType);

        void YieldThread(lua_State *L,
                         std::function<void(std::shared_ptr<RbxStu::StuLuau::YieldRequest>)> runForYield);
    };
} // RbxStu::Luau
