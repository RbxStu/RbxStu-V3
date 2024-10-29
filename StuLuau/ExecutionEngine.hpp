//
// Created by Dottik on 22/10/2024.
//

#pragma once
#include <functional>
#include <future>
#include <queue>
#include <nlohmann/json.hpp>

#include "Environment/EnvironmentContext.hpp"

namespace RbxStu::Scheduling {
    struct ExecutionEngineInitializationInformation;
}

namespace RbxStu::StuLuau {
    enum class ExecutionSecurity {
        LocalScript,
        RobloxScript,
        Plugin,
        RobloxPlugin,
        RobloxExecutor
    };

    enum class ExecutionEngineStep : std::int32_t {
        YieldStep,
        ExecuteStep,
        SynchronizedDispatch,
    };

    struct AssociatedObject {
        std::function<void()> freeObject;
    };

    struct DispatchRequest {
        std::function<void(lua_State *L)> execute;
        RbxStu::StuLuau::ExecutionSecurity executionSecurity;
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
        std::shared_future<void> lpRunningTask;
    };

    struct ExecuteRequest {
        bool bGenerateNativeCode;
        bool bCreateNewThread;
        std::string szLuauCode;
        RbxStu::StuLuau::ExecutionSecurity executeWithSecurity;
    };

    class ExecutionEngine final {
        std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> m_executionEngineState;

        std::queue<std::shared_ptr<RbxStu::StuLuau::YieldRequest> > m_yieldQueue;
        std::queue<RbxStu::StuLuau::ExecuteRequest> m_executeQueue;
        std::shared_ptr<Environment::EnvironmentContext> m_environmentContext;

        std::vector<std::shared_ptr<AssociatedObject> > m_associatedObjects;

        std::atomic_bool m_bIsReadyStepping;
        bool m_bCanUseCodeGeneration;
        std::int8_t m_firstStepsCount;
        std::queue<RbxStu::StuLuau::DispatchRequest> m_synchronizedDispatch;
        lua_State *m_pDispatchThread;
        std::atomic_bool m_bIsDestroyed;

    public:
        explicit ExecutionEngine(
            std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> parentJobInitializationInformation);

        void DestroyEngine();

        ~ExecutionEngine();

        void Execute(const ExecuteRequest &executeRequest);

        void AssociateObject(const std::shared_ptr<AssociatedObject> &associatedObject);

        std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> GetInitializationInformation();

        void SetExecuteReady(bool isReady);

        void StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep stepType);

        void ResumeThread(lua_State *L, int nret);

        void YieldThread(lua_State *L,
                         std::function<void(std::shared_ptr<RbxStu::StuLuau::YieldRequest>)> runForYield,
                         bool bRunInParallel);

        void SetEnvironmentContext(const std::shared_ptr<Environment::EnvironmentContext> &shared);

        void DispatchSynchronized(std::function<void(lua_State *)> callback);

        void ScheduleExecute(
            bool bGenerateNativeCode,
            std::string_view szLuauCode,
            RbxStu::StuLuau::ExecutionSecurity executeWithSecurity, bool bCreateNewThread
        );

        std::shared_ptr<Environment::EnvironmentContext> GetEnvironmentContext();
    };
} // RbxStu::Luau
