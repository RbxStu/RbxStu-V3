//
// Created by Dottik on 14/10/2024.
//

#include "ExecuteScriptJob.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "luacode.h"
#include "lualib.h"
#include "Luau/BytecodeBuilder.h"
#include "Luau/Compiler.h"
#include "Luau/Compiler/src/BuiltinFolding.h"
#include "Roblox/DataModel.hpp"
#include "Roblox/ScriptContext.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool ExecuteScriptJob::ShouldReinitialize(void *job) {
        // Assuming 'job' is WaitingHybridScriptsJob...
        auto scriptContext = RbxStu::Roblox::ScriptContext::FromWaitingHybridScriptsJob(job);

        if (const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
            this->m_stateMap.contains(dataModel->GetDataModelType()) && this->m_stateMap.
                                                                              at(dataModel->GetDataModelType())->
                                                                              dataModel->GetRbxPointer() == dataModel->
            GetRbxPointer())
            return false; // Already initialized

        return true;
    }

    void ExecuteScriptJob::InitializeForDataModel(void *job) {
        // Assuming 'job' is WaitingHybridScriptsJob...
        auto scriptContext = RbxStu::Roblox::ScriptContext::FromWaitingHybridScriptsJob(job);
        auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        if (this->m_stateMap.contains(dataModel->GetDataModelType()) && this->m_stateMap.
                                                                              at(dataModel->GetDataModelType())->
                                                                              dataModel->GetRbxPointer() == dataModel->
            GetRbxPointer()) {
            return; // Already initialized
        }

        const auto globalState = scriptContext->GetGlobalState();

        const auto rL = lua_newthread(globalState);
        lua_ref(globalState, -1);
        lua_pop(globalState, 1);
        const auto nL = lua_newthread(rL);
        lua_ref(rL, -1);
        lua_pop(rL, 1);

        const auto initData = std::make_shared<ExecuteScriptJobInitializationData>();
        initData->globalState = rL;
        initData->executorState = nL;
        initData->scriptContext = scriptContext;
        initData->dataModel = dataModel;

        if (!this->m_executionQueue.contains(dataModel->GetDataModelType())) {
            // Initialize execution queue for this data model

            auto datamodelExecutionQueue = std::queue<ExecuteJobRequest>();
            this->m_executionQueue[dataModel->GetDataModelType()] = datamodelExecutionQueue;
        } else {
            // Clear the queue on reinitialization
            auto datamodelExecutionQueue = this->m_executionQueue[dataModel->GetDataModelType()];

            while (!datamodelExecutionQueue.empty()) {
                datamodelExecutionQueue.pop();
            }
        }

        this->m_stateMap[dataModel->GetDataModelType()] = initData;
    }

    std::optional<std::shared_ptr<ExecuteScriptJob::ExecuteScriptJobInitializationData>>
    ExecuteScriptJob::GetInitializationContext(void *job) {
        auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        if (this->m_stateMap.contains(dataModel->GetDataModelType()) && this->m_stateMap.
                                                                              at(dataModel->GetDataModelType())->
                                                                              dataModel->GetRbxPointer() == dataModel->
            GetRbxPointer()) {
            return this->m_stateMap.at(dataModel->GetDataModelType()); // Already initialized
        }

        return {};
    }

    ExecuteScriptJob::~ExecuteScriptJob() = default;

    bool ExecuteScriptJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                      RBX::TaskScheduler::Job::Stats *jobStats) {
        // This Job always executes as long as the Job is WaitingHybridScriptsJob.
        return jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob;
    }

    void ExecuteScriptJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                RbxStu::Scheduling::TaskScheduler *scheduler) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
        if (this->ShouldReinitialize(job)) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_Jobs_ExecuteScriptJob,
                      std::format("Initializing execution engine for DataModel: {}" ,RBX::DataModelTypeToString(
                          dataModel->GetDataModelType())));
            this->InitializeForDataModel(job);
        }

        auto initContext = this->GetInitializationContext(job);

        if (!initContext.has_value()) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Scheduling_Jobs_ExecuteScriptJob, std::format(
                          "DataModel initialization information lost! Forcing reinitialization for DataModel: {}...",RBX
                          ::DataModelTypeToString(dataModel->GetDataModelType())));
            for (auto init = this->m_stateMap.begin(); init != this->m_stateMap.end(); ++init) {
                if (init->first == dataModel->GetDataModelType()) {
                    this->m_stateMap.erase(init);
                }
            }

            return;
        }

        const auto datamodelType = initContext.value()->dataModel->GetDataModelType();

        if (!m_executionQueue.empty() &&
            m_executionQueue.contains(datamodelType) &&
            !m_executionQueue[datamodelType].empty()) {
            const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
                RbxStuOffsets::GetSingleton()->
                GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));
            auto opts = Luau::CompileOptions{};
            opts.debugLevel = 2;
            opts.optimizationLevel = 1;

            auto &executionQueue = m_executionQueue[datamodelType];

            while (!executionQueue.empty()) {
                auto [bGenerateNativeCode, scriptSource] = executionQueue.front();

                const auto nL = lua_newthread(initContext.value()->executorState);
                luaL_sandboxthread(nL);

                const auto bytecode = Luau::compile(scriptSource.data(), opts);

                luau_load(nL, "RbxStuV3", bytecode.data(), bytecode.size(), 0);
                task_defer(nL);
                lua_pop(initContext.value()->executorState, 1);

                executionQueue.pop();
            }
        }

        return Job::Step(job, jobStats, scheduler);
    }
}
