//
// Created by Dottik on 22/10/2024.
//

#pragma once
#include <functional>
#include <nlohmann/json.hpp>

#include "Environment/EnvironmentContext.hpp"
#include "Scheduling/Job/ExecuteScriptJob.hpp"

namespace RbxStu::Scheduling {
    struct ExecutionEngineInitializationInformation;
}

namespace RbxStu::StuLuau {
    enum class ExecutionEngineStep : std::int32_t {
        YieldStep,
        ExecuteStep,
    };

    enum class ExecutionSecurity {
        LocalScript,
        RobloxScript,
        Plugin,
        RobloxPlugin,
        RobloxExecutor
    };

    struct YieldResult {
        bool bIsSuccess{};
        std::int32_t dwNumberOfReturns{};
        std::optional<std::string> szErrorMessage;
    };

    struct YieldRequest {
        bool bIsReady;
        // should be an std::atomic_bool, but thanks to the fun bits of fucking C++ STL it doesn't WORK, thank you for deleting the ONLY constructor I need.
        lua_State *lpResumeTarget;
        RBX::Lua::WeakThreadRef threadRef;
        std::function<YieldResult()> fpCompletionCallback;
    };

    struct ExecuteRequest {
        bool bGenerateNativeCode;
        std::string szLuauCode;
        RbxStu::StuLuau::ExecutionSecurity executeWithSecurity;
    };

    class ExecutionEngine final {
        std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> m_executionEngineState;

        std::queue<std::shared_ptr<RbxStu::StuLuau::YieldRequest> > m_yieldQueue;
        std::queue<RbxStu::StuLuau::ExecuteRequest> m_executeQueue;
        std::shared_ptr<Environment::EnvironmentContext> m_environmentContext;

        std::atomic_bool m_bIsReadyStepping;
        bool m_bCanUseCodeGeneration;

        void Execute(const ExecuteRequest &execute_request);

    public:
        explicit ExecutionEngine(
            std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> parentJobInitializationInformation);

        std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> GetInitializationInformation();

        void SetExecuteReady(bool isReady);

        void StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep stepType);

        void YieldThread(lua_State *L,
                         std::function<void(std::shared_ptr<RbxStu::StuLuau::YieldRequest>)> runForYield);

        void SetEnvironmentContext(const std::shared_ptr<Environment::EnvironmentContext> &shared);

        void ScheduleExecute(
            bool bGenerateNativeCode,
            std::string_view szLuauCode,
            RbxStu::StuLuau::ExecutionSecurity executeWithSecurity
        );
    };
} // RbxStu::Luau
