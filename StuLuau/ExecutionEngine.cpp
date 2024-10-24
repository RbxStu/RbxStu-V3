//
// Created by Dottik on 22/10/2024.
//

#include "ExecutionEngine.hpp"

#include <string>

namespace RbxStu::StuLuau {
    ExecutionEngine::ExecutionEngine(
        std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> parentJobInitializationInformation) {
        this->m_executionEngineState = parentJobInitializationInformation;
    }

    std::shared_ptr<Scheduling::ExecuteScriptJobInitializationInformation> ExecutionEngine::
    GetInitializationInformation() {
        return this->m_executionEngineState;
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
                }

                break;
            }

            case ExecutionEngineStep::ExecuteStep: {
                // The execute step will dequeue luau scripts and run them through the ROBLOX scheduler

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
