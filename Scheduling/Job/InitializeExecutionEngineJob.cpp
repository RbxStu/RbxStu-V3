//
// Created by Dottik on 24/10/2024.
//

#include "InitializeExecutionEngineJob.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "lualib.h"
#include "Roblox/DataModel.hpp"
#include "Roblox/ScriptContext.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/Environment/EnvironmentContext.hpp"

#include "StuLuau/Environment/UNC/Closures.hpp"
#include "StuLuau/Environment/UNC/Globals.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool InitializeExecutionEngineJob::ShouldStep(const RbxStu::Scheduling::JobKind jobKind, void *job,
                                                  RBX::TaskScheduler::Job::Stats *jobStats) {
        try {
            const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
            const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
            const auto engine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

            if (!dataModel->IsDataModelOpen()) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                          "Not stepping on a closed DataModel pointer.");
                taskScheduler->ResetExecutionEngine(dataModel->GetDataModelType());
                return false; // if DataModel is closed, we should not step this job.
            }


            if (engine != nullptr) {
                const auto engineDataModel = engine->GetInitializationInformation()->dataModel;


                if (engineDataModel->GetDataModelType() == dataModel->GetDataModelType() && engineDataModel->
                    GetRbxPointer()
                    != dataModel->GetRbxPointer()) {
                    // DataModel has re-initialized.
                    taskScheduler->ResetExecutionEngine(dataModel->GetDataModelType());
                    return jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob;
                }

                return false;
            }

            return engine == nullptr && jobKind == RbxStu::Scheduling::JobKind::WaitingHybridScriptsJob;
        } catch (const std::exception &ex) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                      std::format("InitializeExecutionEngine::ShouldStep Failure {}", ex.what()));
        }

        return false;
    }

    Jobs::AvailableJobs InitializeExecutionEngineJob::GetJobIdentifier() {
        return Jobs::AvailableJobs::InitializeExecutionEngineJob;
    }

    void InitializeExecutionEngineJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                                            RbxStu::Scheduling::TaskScheduler *scheduler) {
        const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);

        if (!dataModel->IsDataModelOpen()) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                      "Dropping Initialization on closed DataModel pointer.");
            return; // if DataModel is closed, we should not step this job, REGARDLESS.
        }

        const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto engine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

        /*
         *  We make a few assumptions here:
         *      - If we got to Step, that means that we MUST re-create the ExecutionEngine for the DataModelType this DataModelJob belongs to.
         *      - We are RBX::ScriptContextFacets::WaitingHybridScriptsJob.
         */

        // Assuming 'job' is WaitingHybridScriptsJob...
        const auto scriptContext = RbxStu::Roblox::ScriptContext::FromWaitingHybridScriptsJob(job);

        if (nullptr == scriptContext) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                      "RBX::ScriptContext invalid on WaitingHybridScriptsJob?");
            return;
        }

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  "RBX::ScriptContext::getGlobalState");
        const auto globalState = scriptContext->GetGlobalState();

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  "lua_newthread(global)");
        const auto rL = lua_newthread(globalState);
        lua_ref(globalState, -1);
        lua_pop(globalState, 1);
        const auto nL = lua_newthread(globalState);
        lua_ref(globalState, -1);
        lua_pop(globalState, 1);

        luaL_sandboxthread(nL); // Sandbox to make renv != genv.

        const auto initData = std::make_shared<ExecutionEngineInitializationInformation>();
        initData->globalState = rL;
        initData->executorState = nL;
        initData->scriptContext = scriptContext;
        initData->dataModel = dataModel;

        taskScheduler->CreateExecutionEngine(dataModel->GetDataModelType(), initData);
        const auto executionEngine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Created RbxStu::StuLuau::ExecutionEngine for DataModel {}!", RBX::
                      DataModelTypeToString(
                          dataModel->GetDataModelType())));

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Pushing Environment for DataModel Executor State {}...", RBX
                      ::
                      DataModelTypeToString(
                          dataModel->GetDataModelType())));

        const auto envContext = std::make_shared<StuLuau::Environment::EnvironmentContext>(executionEngine);
        executionEngine->SetEnvironmentContext(envContext);

        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Closures>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Globals>());

        envContext->DefineInitScript(R"(


        )", "InstanceFunctionIntialization");

        envContext->PushEnviornment();

        executionEngine->SetExecuteReady(true);
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Environment pushed to DataModel {}", RBX
                      ::
                      DataModelTypeToString(
                          dataModel->GetDataModelType())));
    }
} // RbxStu::Scheduling::Jobs
