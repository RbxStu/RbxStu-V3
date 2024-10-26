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

struct HookChainInformation {
    std::shared_ptr<RbxStu::Roblox::DataModel> executionEngineDataModel;
    std::map<Closure *, std::list<std::function<std::int32_t(lua_State *)> > >
    hookMap;
    std::map<Closure *, lua_CFunction> originalChain;
    lua_State *mainthread;
};

std::map<RBX::DataModelType, HookChainInformation> s_hookChain;

namespace RbxStu::StuLuau::Environment {
    static int metamethod_thunk(lua_State *L) {
        const auto main = lua_mainthread(L);

        const auto currentFunction = static_cast<Closure *>(L->ci->func->value.p);
        lua_pop(L, 1);
        for (auto &[_dataModelType, hookChainInfo]: s_hookChain) {
            if (main != hookChainInfo.mainthread) {
                continue;
            }

            for (const auto &next: hookChainInfo.hookMap[currentFunction]) {
                // Traverse hook chain from head to bottom, executing every call on the defined order, allows for hooks to be many, whilst simple.
                const auto previousStackSize = lua_gettop(L);
                next(L);
                // Reset Stack.
                lua_settop(L, previousStackSize);
            }

            // Call and return with original call.
            return hookChainInfo.originalChain[currentFunction](L);
        }

        throw std::exception(
            "RbxStu::StuLuau::Environment::EnvironmentContext: Failed to get original hook chain, this should never happen.");
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
    void EnvironmentContext::DefineDataModelHook(std::string_view szMetamethodName,
                                                 std::function<std::int32_t(lua_State *)>
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

        if (lua_getfield(initInfo->executorState, -1, szMetamethodName.data()) != ::lua_Type::LUA_TFUNCTION) {
            lua_pop(initInfo->executorState, 2); // pop mt + nil
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::EnvironmentContext,
                      "Cannot establish hook-chain: meta method does not have a default implementation, or if it has any, it is not a C closure or Luau Closure!");
            return;
        }

        const auto closure = lua_toclosure(initInfo->executorState, -1);
        lua_pop(initInfo->executorState, 1); // Leave only mt on stack.

        if (!s_hookChain[initInfo->dataModel->GetDataModelType()].hookMap.contains(closure)) {
            /*
             *  The hook map has not been initialized for this metamethod, this means there is no hook implace to restore the original, we must do it ourselves.
             */

            if (!lua_iscfunction(initInfo->executorState, -1)) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::EnvironmentContext,
                          "Cannot establish hook-chain: Metamethod is not a C closure, calling it will result on the call-stack becoming messed up.");
                return;
            }


            s_hookChain[initInfo->dataModel->GetDataModelType()].originalChain[closure] = closure->c.f;

            closure->c.f = metamethod_thunk;
        }

        s_hookChain[initInfo->dataModel->GetDataModelType()].hookMap[closure].emplace_back(func);
    }

    void EnvironmentContext::MakeUnhookable(Closure *closure) {
        this->m_unhookableClosures.emplace_back(closure);
    }

    void EnvironmentContext::PushEnviornment() {
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

            lua_setglobal(L, lib->GetLibraryName());

            if (lib->PushToGlobals()) {
                lua_pushvalue(L, LUA_GLOBALSINDEX);
                luaL_register(L, nullptr, envGlobals);
            }
        }

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::EnvironmentContext, "Normalizing Luau Stack.");
        lua_settop(this->m_parentEngine->GetInitializationInformation()->executorState, oldTop);
    }
} // RbxStu::StuLuau::Environment
