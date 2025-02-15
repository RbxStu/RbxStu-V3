//
// Created by Dottik on 24/10/2024.
//

#include "InitializeExecutionEngineJob.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Roblox/DataModel.hpp"
#include "Roblox/ScriptContext.hpp"
#include "StuLuau/Environment/Custom/Memory.hpp"
#include "StuLuau/Environment/Custom/NewGlobals.hpp"
#include "StuLuau/Environment/EnvironmentContext.hpp"
#include "StuLuau/Environment/UNC/Cache.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/LuauSecurity.hpp"
#include "ltable.h"
#include "lualib.h"

#include "StuLuau/Environment/UNC/Closures.hpp"
#include "StuLuau/Environment/UNC/Crypt.hpp"
#include "StuLuau/Environment/UNC/Debug.hpp"
#include "StuLuau/Environment/UNC/FileSystem.hpp"
#include "StuLuau/Environment/UNC/Globals.hpp"
#include "StuLuau/Environment/UNC/Instances.hpp"
#include "StuLuau/Environment/UNC/Miscellaneous.hpp"
#include "StuLuau/Environment/UNC/Scripts.hpp"
#include "StuLuau/Environment/UNC/WebSocket.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool InitializeExecutionEngineJob::ShouldStep(const RbxStu::Scheduling::JobKind jobKind, void *job,
                                                  RBX::TaskScheduler::Job::Stats *jobStats) {
        try {
            const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
            const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
            const auto engine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

            if (engine != nullptr) {
                const auto engineDataModel = engine->GetInitializationInformation()->dataModel;

                if (engineDataModel->GetDataModelType() == dataModel->GetDataModelType() &&
                    engineDataModel->GetRbxPointer() != dataModel->GetRbxPointer()) {
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                              std::format("DataModel re-initialized to {} from {} in this tick, skipping next step...", (void *) dataModel->GetRbxPointer(),
                                          (void *) engineDataModel->GetRbxPointer()));

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
        const auto engine = scheduler->GetExecutionEngine(dataModel->GetDataModelType());
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob, std::format("Initializing ExecutionEngine on DataModel {}; Job address {}", (void*)dataModel->GetRbxPointer(), job));

        /*
         *  We make a few assumptions here:
         *      - If we got to Step, that means that we MUST re-create the ExecutionEngine for the DataModelType this
         * DataModelJob belongs to.
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

        lua_newtable(nL);
        lua_setglobal(nL, "_G");
        lua_getglobal(nL, "_G"); // isolate shared and _G.
        lua_setglobal(nL, "shared");

        lua_pushvalue(nL, LUA_GLOBALSINDEX);
        lua_setglobal(nL, "_ENV");

        const auto initData = std::make_shared<ExecutionEngineInitializationInformation>();
        initData->Lglobal = rL->global;
        initData->globalState = rL;
        initData->executorState = nL;
        initData->scriptContext = scriptContext;
        initData->dataModel = dataModel;

        scheduler->CreateExecutionEngine(dataModel->GetDataModelType(), initData);
        const auto executionEngine = scheduler->GetExecutionEngine(dataModel->GetDataModelType());
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Created RbxStu::StuLuau::ExecutionEngine for DataModel {}!",
                              RBX::DataModelTypeToString(dataModel->GetDataModelType())));

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Pushing Environment for DataModel Executor State {}...",
                              RBX ::DataModelTypeToString(dataModel->GetDataModelType())));

        const auto envContext = std::make_shared<StuLuau::Environment::EnvironmentContext>(executionEngine);
        executionEngine->SetEnvironmentContext(envContext);

        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Crypt>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Cache>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::FileSystem>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Closures>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Debug>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Globals>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::WebSocket>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Scripts>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Miscellaneous>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Instances>());

        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::Custom::Memory>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::Custom::NewGlobals>());

        const static auto s_BannedServices = std::vector<std::string_view>{"linkingservice",
                                                                           "browserservice",
                                                                           "httprbxapiservice",
                                                                           "opencloudservice",
                                                                           "messagebusservice",
                                                                           "omnirecommendationsservice",
                                                                           "captureservice",
                                                                           "corepackages",
                                                                           "animationfromvideocreatorservice",
                                                                           "safetyservice",
                                                                           "appupdateservice",
                                                                           "ugcvalidationservice",
                                                                           "accountservice",
                                                                           "analyticsservice",
                                                                           "ixpservice",
                                                                           "commerceservice",
                                                                           "sessionservice",
                                                                           "studioservice",
                                                                           "platformcloudstorageservice",
                                                                           "startpageservice",
                                                                           "scripteditorservice",
                                                                           "avatareditorservice",
                                                                           "webviewservice",
                                                                           "commerceservice"};

        envContext->DefineDataModelHook(
                "__namecall",
                [](const StuLuau::Environment::HookInputState &inCtx) -> StuLuau::Environment::HookReturnState {
                    const auto currentNamecall = lua_namecallatom(inCtx.L, nullptr);

                    if (currentNamecall == nullptr)
                        return StuLuau::Environment::HookReturnState{true, false, 0};

                    const auto luauSecurity = StuLuau::LuauSecurity::GetSingleton();
                    const auto bIsServiceRetrieval =
                            strcmp(currentNamecall, "GetService") == 0 || strcmp(currentNamecall, "service") == 0 ||
                            strcmp(currentNamecall, "FindService") == 0 || strcmp(currentNamecall, "getService") == 0;

                    if (bIsServiceRetrieval && luauSecurity->IsOurThread(inCtx.L)) {
                        const auto target = Utilities::ToLower(lua_tostring(inCtx.L, 2));
                        for (const auto &service: s_BannedServices) {
                            if (target == service)
                                luaL_error(inCtx.L, "this service has been blocked for safety reasons");
                        }
                    }

                    return StuLuau::Environment::HookReturnState{true, false, 0};
                });

        envContext->DefineDataModelHook(
                "__namecall",
                [](const StuLuau::Environment::HookInputState &inCtx) -> StuLuau::Environment::HookReturnState {
                    const auto currentNamecall = lua_namecallatom(inCtx.L, nullptr);

                    if (currentNamecall == nullptr)
                        return StuLuau::Environment::HookReturnState{true, false, 0};

                    if (strcmp(currentNamecall, "HttpGet") == 0 || strcmp(currentNamecall, "HttpGetAsync") == 0) {
                        lua_getglobal(inCtx.L, "httpget");
                        lua_pushvalue(inCtx.L, 2);
                        lua_pcall(inCtx.L, 1, 1, 0);

                        return StuLuau::Environment::HookReturnState{false, true, 1};
                    }

                    return StuLuau::Environment::HookReturnState{true, false, 0};
                });

        envContext->DefineDataModelHook(
                "__index",
                [](const StuLuau::Environment::HookInputState &inCtx) -> StuLuau::Environment::HookReturnState {
                    if (lua_type(inCtx.L, 2) != ::lua_Type::LUA_TSTRING)
                        return StuLuau::Environment::HookReturnState{true, false, 0};

                    auto idx = lua_tostring(inCtx.L, 2);

                    if (strcmp(idx, "HttpGet") == 0) {
                        lua_getglobal(inCtx.L, "httpget");
                        return StuLuau::Environment::HookReturnState{false, false, 1};
                    }

                    return StuLuau::Environment::HookReturnState{true, false, 0};
                });

        envContext->DefineInitScript(R"(
            local getconnections = closures.clonefunction(getconnections)
            local newcclosure = closures.clonefunction(closures.newcclosure)
            local getgenv = closures.clonefunction(uncrbxstu.getgenv)
            local typeof = closures.clonefunction(typeof)
            local rawget = closures.clonefunction(rawget)
            local pcall = closures.clonefunction(pcall)
            local getreg = closures.clonefunction(getreg)
            local getinstancelist = closures.clonefunction(getinstancelist)

            getgenv().getinstances = newcclosure(function()
                local x = {}

                for _, insn in getinstancelist() do
                    if typeof(insn) == "Instance" and insn.Parent then
                        table.insert(x, insn)
                    end
                end

                return x
            end)
            getgenv().getscripts = newcclosure(function()
                local x = {}
                for _, insn in getinstancelist() do
                    if typeof(insn) == "Instance" and insn:IsA("LuaSourceContainer") then
                        table.insert(x, insn)
                    end
                end

                return x
            end)

            getgenv().getnilinstances = newcclosure(function()
                local x = {}

                for _, insn in getinstancelist() do
                    if typeof(insn) == "Instance" and insn.Parent then
                        continue
                    end
                    table.insert(x, insn)
                end

                return x
            end)
            getgenv().firesignal = newcclosure(function(signal, ...)
                local cons = getconnections(signal)

                for _, con in cons do
                    if typeof(con.Fire) == "function" then
                        pcall(con.Fire, con, ...)
                    end
                end

                task.wait()
            end)
        )",
                                     "InteropFunctionDeclarations");

        envContext->DefineInitScript(R"(
            setreadonly(getgenv().debug, false)
            for i, e in getrenv().debug do
                getgenv().debug[i] = e
            end
            setreadonly(getgenv().debug, true)
        )",
                                     "DebugRobloxImport");

        envContext->DefineInitScript(R"(
            makeunhookable(getgenv)
            makeunhookable(getrenv)
            makeunhookable(isunhookable)
            makeunhookable(makeunhookable)
            makeunhookable(ishooked)
            makeunhookable(restorefunction)
            makeunhookable(hookfunction)
            makeunhookable(hookmetamethod)
            makeunhookable(clonefunction)

            local task_wait = clonefunction(task.wait)

            local function deepclone(t, rec)
                if rec > 20 then return t end   -- Likely ref chain.

                local nTable = {}
                for idx, val in t do
                    if typeof(val) == "table" and val ~= t then
                        nTable[idx] = deepclone(val, rec + 1)
                    elseif typeof(val) == "table" then
                        nTable[idx] = nTable
                    end

                    nTable[idx] = val
                end

                return nTable
            end

            local realGenv = deepclone(getgenv(), 0)
            local makeunhookable = clonefunction(makeunhookable)

            -- Crypt functions are unhookable by default.
            for _, func in getgenv().crypt do
                makeunhookable(func)
            end

            makeunhookable(httpget)
            makeunhookable(closures.loadstring)

            -- Prevent overwrites into getgenv (watcher, metamethods proved to be unwise here)
            task.spawn(newcclosure(function()
                while task_wait() do
					local genv = getgenv()
                    for idx, func in realGenv do
                        if typeof(func) == "table" then
                            for jdx, jfunc in realGenv[idx] do
                                if realGenv[idx][jdx] ~= genv[idx][jdx] then
                                    genv[idx][jdx] = jfunc
                                end
                            end
                        end
                        if realGenv[idx] ~= genv[idx] then
                            genv[idx] = func
                        end
                    end
                end
            end))

        )",
                                     "UnhookableSecurity");

        envContext->PushEnvironment();

        executionEngine->SetExecuteReady(true);
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Environment pushed to DataModel {}",
                              RBX ::DataModelTypeToString(dataModel->GetDataModelType())));
    }
} // namespace RbxStu::Scheduling::Jobs
