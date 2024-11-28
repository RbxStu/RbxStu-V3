//
// Created by Dottik on 24/10/2024.
//

#include "InitializeExecutionEngineJob.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "ltable.h"
#include "lualib.h"
#include "Roblox/DataModel.hpp"
#include "Roblox/ScriptContext.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/LuauSecurity.hpp"
#include "StuLuau/Environment/EnvironmentContext.hpp"
#include "StuLuau/Environment/Custom/Memory.hpp"
#include "StuLuau/Environment/Custom/NewGlobals.hpp"
#include "StuLuau/Environment/UNC/Cache.hpp"

#include "StuLuau/Environment/UNC/Closures.hpp"
#include "StuLuau/Environment/UNC/Crypt.hpp"
#include "StuLuau/Environment/UNC/Debug.hpp"
#include "StuLuau/Environment/UNC/Globals.hpp"
#include "StuLuau/Environment/UNC/Scripts.hpp"
#include "StuLuau/Environment/UNC/WebSocket.hpp"
#include "StuLuau/Environment/UNC/Miscellaneous.hpp"

namespace RbxStu::Scheduling::Jobs {
    bool InitializeExecutionEngineJob::ShouldStep(const RbxStu::Scheduling::JobKind jobKind, void *job,
                                                  RBX::TaskScheduler::Job::Stats *jobStats) {
        try {
            const auto dataModel = RbxStu::Roblox::DataModel::FromJob(job);
            const auto taskScheduler = TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
            const auto engine = taskScheduler->GetExecutionEngine(dataModel->GetDataModelType());

            if (engine != nullptr) {
                const auto engineDataModel = engine->GetInitializationInformation()->dataModel;

                if (engineDataModel->GetDataModelType() == dataModel->GetDataModelType() && engineDataModel->
                    GetRbxPointer()
                    != dataModel->GetRbxPointer()) {
                    RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                              std::format("DataModel re-initialized to {} from {}", (void*)dataModel->GetRbxPointer(),
                                  (void*)engineDataModel->GetRbxPointer()));

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

        lua_newtable(nL);
        lua_setglobal(nL, "_G");
        lua_getglobal(nL, "_G"); // isolate shared and _G.
        lua_setglobal(nL, "shared");

        lua_pushvalue(nL, LUA_GLOBALSINDEX);
        lua_setglobal(nL, "_ENV");

        const auto initData = std::make_shared<ExecutionEngineInitializationInformation>();
        initData->globalState = rL;
        initData->executorState = nL;
        initData->scriptContext = scriptContext;
        initData->dataModel = dataModel;

        scheduler->CreateExecutionEngine(dataModel->GetDataModelType(), initData);
        const auto executionEngine = scheduler->GetExecutionEngine(dataModel->GetDataModelType());
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

        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Cache>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Closures>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Debug>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Globals>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::WebSocket>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Scripts>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Crypt>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::UNC::Miscellaneous>());

        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::Custom::Memory>());
        envContext->DefineLibrary(std::make_shared<StuLuau::Environment::Custom::NewGlobals>());

        const static auto s_BannedServices = std::vector<std::string_view>{
            "linkingservice",
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
            "commerceservice"
        };

        envContext->DefineDataModelHook("__namecall",
                                        [](const StuLuau::Environment::HookInputState &inCtx) ->
                                    StuLuau::Environment::HookReturnState {
                                            const auto currentNamecall = lua_namecallatom(inCtx.L, nullptr);

                                            if (currentNamecall == nullptr)
                                                return StuLuau::Environment::HookReturnState{true, false, 0};

                                            const auto luauSecurity = StuLuau::LuauSecurity::GetSingleton();
                                            const auto bIsServiceRetrieval =
                                                    strcmp(currentNamecall, "GetService") == 0
                                                    || strcmp(currentNamecall, "service") == 0
                                                    || strcmp(currentNamecall, "FindService") == 0
                                                    || strcmp(currentNamecall, "getService") == 0;

                                            if (bIsServiceRetrieval && luauSecurity->IsOurThread(inCtx.L)) {
                                                const auto target = Utilities::ToLower(lua_tostring(inCtx.L, 2));
                                                for (const auto &service: s_BannedServices) {
                                                    if (target == service)
                                                        luaL_error(
                                                        inCtx.L,
                                                        "this service has been blocked for safety reasons");
                                                }
                                            }

                                            return StuLuau::Environment::HookReturnState{true, false, 0};
                                        });

        envContext->DefineDataModelHook("__namecall",
                                        [](const StuLuau::Environment::HookInputState &inCtx) ->
                                    StuLuau::Environment::HookReturnState {
                                            const auto currentNamecall = lua_namecallatom(inCtx.L, nullptr);

                                            if (currentNamecall == nullptr)
                                                return StuLuau::Environment::HookReturnState{true, false, 0};

                                            if (strcmp(currentNamecall, "HttpGet") == 0) {
                                                lua_getglobal(inCtx.L, "httpget");
                                                lua_pushvalue(inCtx.L, 2);
                                                lua_pcall(inCtx.L, 1, 1, 0);

                                                return StuLuau::Environment::HookReturnState{false, true, 1};
                                            }

                                            return StuLuau::Environment::HookReturnState{true, false, 0};
                                        });

        envContext->DefineDataModelHook("__index",
                                        [](const StuLuau::Environment::HookInputState &inCtx) ->
                                    StuLuau::Environment::HookReturnState {
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
            local newcclosure = closures.clonefunction(closures.newcclosure)
            local getgenv = closures.clonefunction(uncrbxstu.getgenv)
            local typeof = closures.clonefunction(typeof)

            local function getInstanceList(idx: number)
                if not idx then idx = 0 end
                if idx > 30 then
                    -- If this occurs, the DataModel is likely an EDIT or STANDALONE, which means they don't hold a map like this.
                    return game:GetDescendants()
                end

                local part = Instance.new("Part")
                for _, obj in uncrbxstu.getreg() do
                    if typeof(obj) == "table" and rawget(obj, "__mode") == "kvs" then
			            for idx_, inst in obj do
				            if inst == part then
					            part:Destroy()
					            instanceList = obj
                                return instanceList
				            end
			            end
		            end
                end
                part:Destroy()

                task.wait() -- Not much choice, a yield may help us find the instance list
                return getInstanceList(idx + 1)
            end

            getgenv().getinstances = newcclosure(function()
                local x = {}

                for _, insn in getInstanceList() do
                    if typeof(insn) == "Instance" and insn.Parent then
                        table.insert(x, insn)
                    end
                end

                return x
            end)
            getgenv().getscripts = newcclosure(function()
                local x = {}
                for _, insn in getInstanceList() do
                    if typeof(insn) == "Instance" and insn:IsA("LuaSourceContainer") then
                        table.insert(x, insn)
                    end
                end

                return x
            end)

            getgenv().getnilinstances = newcclosure(function()
                local x = {}

                for _, insn in getInstanceList() do
                    if typeof(insn) == "Instance" and insn.Parent then
                        continue
                    end
                    table.insert(x, insn)
                end

                return x
            end)
        )", "InstanceFunctionIntialization");

        envContext->DefineInitScript(R"(
            setreadonly(getgenv().debug, false)
            for i, e in getrenv().debug do
                getgenv().debug[i] = e
            end
            setreadonly(getgenv().debug, true)
        )", "DebugRobloxImport");

        // TODO: Fix safe instances, cannot even paste wave appropriately, smh.

        envContext->DefineInitScript(R"(
			local __safeInstanceMap = table.create(8)

			local Instance_new = clonefunction(Instance.new)
			local compareinstances = clonefunction(compareinstances)
			local newcclosure = clonefunction(newcclosure)
			local checkcaller = clonefunction(checkcaller)
			local select = clonefunction(select)
			local typeof = clonefunction(typeof)
			local pcall = clonefunction(pcall)
			local getnamecallmethod = clonefunction(getnamecallmethod)

			local findFirstChild = clonefunction(game.FindFirstChild)
			local getChildren = clonefunction(game.GetChildren)
			local getDescendants = clonefunction(game.GetDescendants)

			local findFirstChild_hk = newcclosure(function(self: Instance, childName: string, searchDescendants: boolean)
				local child = findFirstChild(self, childName, searchDescendants)

				if checkcaller() then
					return child -- No validation applied, we are OK.
				end

				for _, insn in __safeInstanceMap do
					if compareinstances(insn, child) then
						return nil
					end
				end

				return child
			end)

			local getChildren_hk = newcclosure(function(self: Instance)
				local children = getChildren(self)

				if checkcaller() then
					return children -- No validation applied, we are OK.
				end

				local newChildren = {}

				for _, insn in __safeInstanceMap do
					for _, child in children do
						if not compareinstances(insn, child) then
							table.insert(newChildren, child) -- Not a protected instance.
						end
					end
				end

				return newChildren
			end)

			local getDescendants_hk = newcclosure(function(self: Instance)
				local descendants = getDescendants(self)

				if checkcaller() then
					return descendants -- No validation applied, we are OK.
				end

				local newDescendants = {}

				for _, insn in __safeInstanceMap do
					for _, descendant in descendants do
						if not compareinstances(insn, descendant) then
							table.insert(newDescendants, descendant) -- Not a protected instance.
						end
					end
				end

				return newDescendants
			end)

			local oldIdx
			oldIdx = hookmetamethod(game, "__index", function(...)
				if select("#", ...) ~= 2 then
					return oldIdx(...)
				end
				local self = select(1, ...)
				local idx = select(2, ...)

				if typeof(self) ~= "Instance" then
					return oldIdx(...)
				end

				if idx == "FindFirstChild" then
					return findFirstChild_hk
				elseif idx == "GetChildren" then
					return getChildren_hk
				elseif idx == "GetDescendants" then
					return getDescendants_hk
				end

				local success, isProtectedInstance = pcall(function()
					local x = self[idx]

					if typeof(x) == "Instance" then
						for _, obj in __safeInstanceMap do
							if compareinstances(obj, x) then
								return true
							end
						end
					end

					return false
				end)

				if success and isProtectedInstance and not checkcaller() then
					return nil
				end

				return oldIdx(...)
			end)

			local oldNamecall
			oldNamecall = hookmetamethod(game, "__namecall", function(...)
				if select("#", ...) == 0 then
					return oldNamecall(...)
				end
				local self = select(1, ...)
				local namecall = getnamecallmethod()

				if typeof(self) ~= "Instance" then
					return oldNamecall(...)
				end

				if namecall == "FindFirstChild" then
					return findFirstChild_hk(...)
				elseif namecall == "GetChildren" then
					return getChildren_hk(...)
				elseif namecall == "GetDescendants" then
					return getDescendants_hk(...)
				end

				return oldNamecall(...)
			end)

			getgenv().setsecureinstance = newcclosure(function(instance: Instance)
				for _, insn in __safeInstanceMap do
					if compareinstances(insn, instance) then
						return
					end
				end

				table.insert(__safeInstanceMap, instance)
				instance.Destroying:Connect(function()
					getgenv().setnormalinstance(instance)
				end)
			end)

			getgenv().issecuredinstance = newcclosure(function(instance: Instance)
				for _, insn in __safeInstanceMap do
					if compareinstances(insn, instance) then
						return true
					end
				end

				return false
			end)

			getgenv().setnormalinstance = newcclosure(function(instance: Instance)
				for jdx, insn in __safeInstanceMap do
					if compareinstances(insn, instance) then
						table.remove(__safeInstanceMap, jdx)
					end
				end
			end)

			getgenv().createsecurefolder = newcclosure(function(instance: Instance)
				local fd = Instance_new("Folder")
				fd.DescendantAdded:Connect(function(descendant)
					getgenv().setsecureinstance(descendant)
				end)
				fd.DescendantRemoving:Connect(function(descendant)
					getgenv().setnormalinstance(descendant)
				end)

				getgenv().setsecureinstance(fd)

				return fd
			end)
        )", "SecureInstances");

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

        )", "UnhookableSecurity");

        envContext->PushEnvironment();

        executionEngine->SetExecuteReady(true);
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Scheduling_Jobs_InitializeExecutionEngineJob,
                  std::format("Environment pushed to DataModel {}", RBX
                      ::
                      DataModelTypeToString(
                          dataModel->GetDataModelType())));
    }
} // RbxStu::Scheduling::Jobs
