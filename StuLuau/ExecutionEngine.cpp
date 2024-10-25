//
// Created by Dottik on 22/10/2024.
//

#include "ExecutionEngine.hpp"

#include <Logger.hpp>
#include <string>

#include "lualib.h"
#include "LuauSecurity.hpp"
#include "Luau/CodeGen.h"
#include "Luau/Compiler.h"
#include "Luau/Compiler/src/BuiltinFolding.h"
#include "Roblox/ScriptContext.hpp"

namespace RbxStu::StuLuau {
    ExecutionEngine::ExecutionEngine(
        std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> parentJobInitializationInformation) {
        this->m_executionEngineState = parentJobInitializationInformation;

        if (!Luau::CodeGen::isNativeExecutionEnabled(this->m_executionEngineState->executorState)) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::ExecutionEngine,
                      "Luau Native Code Generation is not enabled on the Luau VM! Enabling without ROBLOX intervention!");
            Luau::CodeGen::create(this->m_executionEngineState->executorState->global->mainthread);
            Luau::CodeGen::setNativeExecutionEnabled(this->m_executionEngineState->executorState->global->mainthread,
                                                     true);
            Luau::CodeGen::setNativeExecutionEnabled(this->m_executionEngineState->executorState, true);

            RbxStuLog(RbxStu::LogType::Warning, RbxStu::ExecutionEngine,
                      "Luau Native Code Generation support has been forcefully enabled on the Luau VM.");
        }
    }

    std::shared_ptr<Scheduling::ExecutionEngineInitializationInformation> ExecutionEngine::
    GetInitializationInformation() {
        return this->m_executionEngineState;
    }

    void ExecutionEngine::Execute(const ExecuteRequest &executeRequest) {
        auto luauSecurity = StuLuau::LuauSecurity::GetSingleton();

        auto L = this->GetInitializationInformation()->executorState;

        auto nL = lua_newthread(L);
        lua_pop(L, 1);
        luaL_sandboxthread(nL);

        luauSecurity->SetThreadSecurity(nL, executeRequest.executeWithSecurity, true);

        const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
            RbxStuOffsets::GetSingleton()->
            GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

        auto opts = Luau::CompileOptions{};
        opts.debugLevel = 2;
        opts.optimizationLevel = 1;


        const auto bytecode = Luau::compile(executeRequest.szLuauCode.data(), opts);

        if (luau_load(nL, "=RbxStuV3", bytecode.data(), bytecode.size(), 0) != LUA_OK) {
            const auto error = lua_tostring(nL, -1);
            RbxStuLog(RbxStu::LogType::Error, RbxStu::ExecutionEngine,
                      std::format("Failed to load bytecode: {}", error));
            lua_error(nL);
            return;
        }

        if (executeRequest.bGenerateNativeCode) {
            Luau::CodeGen::CompilationOptions opts{};
            opts.flags = Luau::CodeGen::CodeGenFlags::CodeGen_ColdFunctions;
            Luau::CodeGen::compile(L, -1, opts);
        }

        task_defer(nL);
    }

    void ExecutionEngine::ExecutionEngine::StepExecutionEngine(RbxStu::StuLuau::ExecutionEngineStep stepType) {
        switch (stepType) {
            case ExecutionEngineStep::YieldStep: {
                // During the yielding stage we want to step over our yield jobs queue and
                // dequeue the next yielding step, if it is not ready we want to reschedule (if there is no other job on the queue we want to leave it be for performance reasons)

                auto frontYield = this->m_yieldQueue.front();

                if (!frontYield->bIsReady) {
                    if (this->m_yieldQueue.empty()) {
                        break;
                    } else {
                        this->m_yieldQueue.pop();
                        this->m_yieldQueue.emplace(frontYield);
                    }
                    break;
                }

                // The yield is ready, we must resume.

                const auto completionResults = frontYield->fpCompletionCallback();

                this->m_executionEngineState->scriptContext->ResumeThread(&frontYield->threadRef, completionResults);

                break;
            }

            case ExecutionEngineStep::ExecuteStep: {
                // The execute step will dequeue luau scripts and run them through the ROBLOX scheduler

                auto frontExec = this->m_executeQueue.front();
                this->m_executeQueue.pop();

                this->Execute(frontExec);

                break;
            }
            default: {
                throw std::exception(std::format("Cannot step ExecutionEngine, unknown step type -> {}",
                                                 static_cast<int>(stepType)).c_str());
            }
        }
    }

    void ExecutionEngine::YieldThread(lua_State *L,
                                      std::function<void(std::shared_ptr<RbxStu::StuLuau::YieldRequest>)> runForYield) {
        lua_pushthread(L);
        const auto threadRef = lua_ref(L, -1);
        lua_pop(L, 1);

        auto yieldRequest = std::make_shared<RbxStu::StuLuau::YieldRequest>(RbxStu::StuLuau::YieldRequest{
                false, L, {0, L, threadRef, 0}, nullptr
            }
        );

        this->m_yieldQueue.emplace(yieldRequest);

        std::thread(runForYield, yieldRequest).detach();
    }
} // RbxStu::Luau
