//
// Created by Dottik on 25/10/2024.
//

#include "EnvironmentContext.hpp"

#include <Logger.hpp>

#include "lapi.h"
#include "Library.hpp"
#include "Roblox/DataModel.hpp"
#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include <unordered_set>

#include "StuLuau/Extensions/luauext.hpp"

struct HookChainInformation {
    std::shared_ptr<RbxStu::Roblox::DataModel> executionEngineDataModel;
    std::map<Closure *, std::shared_ptr<std::list<std::function<RbxStu::StuLuau::Environment::HookReturnState(
        const RbxStu::StuLuau::Environment::HookInputState &state)> > > >
    hookMap;
    std::map<Closure *, std::shared_ptr<lua_CFunction> > originalChain;
    lua_State *mainthread;
};

std::map<RBX::DataModelType, HookChainInformation> s_hookChain;

namespace RbxStu::StuLuau::Environment {
    static int metamethod_thunk(lua_State *L) {
        const auto main = lua_mainthread(L);
        const auto currentFunction = clvalue(L->ci->func);

        for (auto &[_dataModelType, hookChainInfo]: s_hookChain) {
            if (main != hookChainInfo.mainthread) {
                continue;
            }

            for (const auto &next: *hookChainInfo.hookMap[currentFunction]) {
                // Traverse hook chain from head to bottom, executing every call on the defined order, allows for hooks to be many, whilst simple.
                const auto previousStackSize = lua_gettop(L);
                const auto hkInfo = next(RbxStu::StuLuau::Environment::HookInputState{
                    L, *hookChainInfo.originalChain[currentFunction]
                });

                // Reset Stack.
                if (hkInfo.bContinueNextHook)
                    lua_settop(L, previousStackSize); // Reset stack for next hook
                else {
                    if (hkInfo.bInvokeLuaYield)
                        return lua_yield(L, hkInfo.returnCount);

                    return hkInfo.returnCount;
                }
            }

            // Call and return with original call.
            if (!hookChainInfo.originalChain.contains(currentFunction))
                luaL_error(L,
                       "RbxStu::StuLuau::Environment::EnvironmentContext: Failed to get original hook chain, this should never happen.");

            return (*hookChainInfo.originalChain[currentFunction])(L);
        }

        luaL_error(L,
                   "RbxStu::StuLuau::Environment::EnvironmentContext: Failed to get original hook chain, this should never happen.");
    }

    EnvironmentContext::~EnvironmentContext() {
        this->DestroyContext();
    }

    void EnvironmentContext::DefineNewDataModelMetaMethodClosure(Closure *originalMetamethod, Closure *func) const {
        /*
        *  During the first call, we must initialize our s_hookChain map for the DataModel this execution engine is running on.
        */
        const auto initInfo = this->m_parentEngine->GetInitializationInformation();

        s_hookChain[initInfo->dataModel->GetDataModelType()].hookMap[func] = s_hookChain[initInfo->dataModel->
            GetDataModelType()].hookMap[originalMetamethod];

        s_hookChain[initInfo->dataModel->GetDataModelType()].originalChain[func] = s_hookChain[initInfo->dataModel->
            GetDataModelType()].originalChain[originalMetamethod];
    }

    void EnvironmentContext::DestroyContext() {
        if (this->m_bIsDestroyed) return;
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Tainting EnvironmentContext...");
        this->m_bIsDestroyed = true;
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Clearing newcclosures...");
        this->m_newcclosures.clear();
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Clearing function hooks...");
        this->m_functionHooks.clear();
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Clearing initscript chain...");
        this->m_initScripts.clear();
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Clearing unhookable closures...");
        this->m_unhookableClosures.clear();
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Unreferencing parent engine...");
        this->m_parentEngine = nullptr;

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "EnvironmentContext Destroyed!");
    }

    void EnvironmentContext::DefineInitScript(const std::string &scriptSource, const std::string &scriptName) {
        this->m_initScripts.emplace_back(scriptSource, scriptName);
    }

    void EnvironmentContext::DefineLibrary(const std::shared_ptr<RbxStu::StuLuau::Environment::Library> &library) {
        for (const auto &lib: this->m_libraries) {
            if (strcmp(lib->GetLibraryName(), library->GetLibraryName()) == 0) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::EnvironmentContext,
                          std::format("Library already defined, ignoring re-definition. Affected Library: {}", library->
                              GetLibraryName()));
                return;
            }
        }

        this->m_libraries.emplace_back(library);
    }

    // It is not really const, modifies static mutable state, shut up ReSharper++ :)
    // ReSharper disable once CppMemberFunctionMayBeConst
    void EnvironmentContext::DefineDataModelHook(const std::string_view szMetamethodName,
                                                 std::function<HookReturnState(const HookInputState &)>
                                                 func) {
        /*
         *  During the first call, we must initialize our s_hookChain map for the DataModel this execution engine is running on.
         */
        const auto initInfo = this->m_parentEngine->GetInitializationInformation();
        if (s_hookChain.contains(initInfo->dataModel->GetDataModelType())) {
            // DataModel pointers must match, else we must void the map entirely and re-create it.
            if (s_hookChain.at(initInfo->dataModel->GetDataModelType()).executionEngineDataModel->GetRbxPointer() !=
                initInfo->dataModel->GetRbxPointer()) {
                auto hkChain = s_hookChain[initInfo->dataModel->GetDataModelType()];
                // DataModel pointer is out of date, void map and re-create.
                hkChain.originalChain.erase(
                    hkChain.originalChain.begin(),
                    hkChain.originalChain.end()); // Clear.

                hkChain.executionEngineDataModel = nullptr;
                hkChain.executionEngineDataModel = initInfo->dataModel;
                hkChain.mainthread = lua_mainthread(initInfo->executorState);

                s_hookChain[initInfo->dataModel->GetDataModelType()] = hkChain;
            }
        } else {
            s_hookChain[initInfo->dataModel->GetDataModelType()] = {
                initInfo->dataModel,
                {},
                {},
                lua_mainthread(initInfo->globalState)
            };
        }


        lua_getglobal(initInfo->executorState, "game");
        lua_getmetatable(initInfo->executorState, -1);
        lua_remove(initInfo->executorState, -2);

        if (lua_getfield(initInfo->executorState, -1, szMetamethodName.data()) != ::lua_Type::LUA_TFUNCTION) {
            lua_pop(initInfo->executorState, 2); // pop mt + nil
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::EnvironmentContext,
                      "Cannot establish hook-chain: meta method does not have a default implementation, or if it has any, it is not a C closure or Luau Closure!");
            return;
        }

        const auto closure = lua_tomutclosure(initInfo->executorState, -1);

        if (!s_hookChain[initInfo->dataModel->GetDataModelType()].hookMap.contains(closure)) {
            /*
             *  The hook map has not been initialized for this metamethod, this means there is no hook implace to restore the original, we must do it ourselves.
             */

            auto iscclosure = lua_iscfunction(initInfo->executorState, -1);

            if (!iscclosure) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::EnvironmentContext,
                          "Cannot establish hook-chain: Metamethod is not a C closure, calling it will result on the call-stack becoming messed up.");
                return;
            }

            s_hookChain[initInfo->dataModel->GetDataModelType()].hookMap[closure] = std::make_shared<std::list<
                std::function<RbxStu::StuLuau::Environment::HookReturnState(
                    const RbxStu::StuLuau::Environment::HookInputState &state)> > >();

            s_hookChain[initInfo->dataModel->GetDataModelType()].originalChain[closure] = std::make_shared<
                lua_CFunction>(closure->c.f);

            RbxStuLog(RbxStu::LogType::Information, RbxStu::EnvironmentContext,
                      "Hijacked Roblox's metamethod with a Proxy!");
            closure->c.f = metamethod_thunk;
        }

        RbxStuLog(RbxStu::LogType::Information, RbxStu::EnvironmentContext,
                  "Pushed new hook to the hook chain...");

        s_hookChain[initInfo->dataModel->GetDataModelType()].hookMap[closure]->emplace_back(func);

        lua_pop(initInfo->executorState, 2); // Leave nothing on the lua stack.
    }

    void EnvironmentContext::MakeUnhookable(Closure *closure) {
        this->m_unhookableClosures.insert(closure);
    }

    bool EnvironmentContext::IsUnhookable(Closure *closure) const {
        if (this->m_unhookableClosures.contains(closure))
            return true;

        // Make a deeper check
        for (const auto &func: this->m_unhookableClosures) {
            if (func->isC != closure->isC)
                continue;

            if (func->isC && closure->c.f == func->c.f)
                return true; // Main C closure is unhookable.

            if (!func->isC && closure->l.p == func->l.p)
                return true; // Main Luau closure is unhookable.
        }

        return false;
    }

    void EnvironmentContext::PushEnvironment() {
        /*
         *  Note: Unbalanced Luau Stacks are bad. do NOT pop like an idiot, just save the previous top and then pop it all out, its MUCH easier
         *  like that, sudden crashes are crazy, though.
         */

        const auto L = this->m_parentEngine->GetInitializationInformation()->executorState;
        const auto oldTop = lua_gettop(this->m_parentEngine->GetInitializationInformation()->executorState);
        for (const auto &lib: this->m_libraries) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext,
                      std::format("Pushing library {} to the environment.",lib->GetLibraryName()));

            const auto envGlobals = lib->GetFunctionRegistry();
            lua_newtable(L);
            luaL_register(L, nullptr, envGlobals);
            lua_setreadonly(L, -1, true);

            if (lua_getglobal(L, lib->GetLibraryName()) != ::lua_Type::LUA_TNIL)
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext,
                      std::format(
                          "DEV WARN: Currently over-writing already defined global library {} with a new registry, this may be intended, but this is important that if callers expect the methods of the previous library to be present scripts may not run as expected anymore!"
                          ,
                          lib->GetLibraryName()));

            lua_pop(L, 1); // pop prev lua_getglobal.

            lua_setglobal(L, lib->GetLibraryName());

            if (lib->PushToGlobals()) {
                lua_pushvalue(L, LUA_GLOBALSINDEX);
                luaL_register(L, nullptr, envGlobals);
                lua_pop(L, 1); // Does not pop stack, must do it ourselves.
            }
        }

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Normalizing Luau Stack.");
        lua_settop(L, oldTop);

        RbxStuLog(RbxStu::LogType::Information, RbxStu::EnvironmentContext, "Executing Init Scripts...");

        for (const auto &init: this->m_initScripts) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::EnvironmentContext,
                      std::format("- RbxStuV3/init/{}.luau", init.scriptName));

            m_parentEngine->Execute(
                {true, true, init.scriptSource, RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor});
        }
    }

    bool EnvironmentContext::IsWrappedClosure(Closure *cl) const {
        return cl->isC && this->m_newcclosures.contains(cl);
    }

    bool EnvironmentContext::IsDataModelMetamethod(Closure *closure) const {
        const auto initInfo = this->m_parentEngine->GetInitializationInformation();

        if (!s_hookChain.contains(initInfo->dataModel->GetDataModelType()))
            return false;

        return s_hookChain.at(initInfo->dataModel->GetDataModelType()).originalChain.contains(closure);
    }
} // RbxStu::StuLuau::Environment
