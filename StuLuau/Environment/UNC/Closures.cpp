//
// Created by Dottik on 25/10/2024.
//

#include "Closures.hpp"

#include <Utilities.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "lapi.h"
#include "lfunc.h"
#include "lgc.h"

#include "lobject.h"
#include "lstate.h"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Closures::iscclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_pushboolean(L, lua_iscfunction(L, 1));
        return 1;
    }

    int Closures::islclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_pushboolean(L, !lua_iscfunction(L, 1));
        return 1;
    }

    int Closures::clonefunction(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);

        if (!lua_iscfunction(L, 1)) {
            lua_clonefunction(L, 1);
            return 1;
        }

        const auto originalCl = lua_toclosure(L, 1);

        Closure *newcl = luaF_newCclosure(L, originalCl->nupvalues, originalCl->env);

        if (originalCl->c.debugname != nullptr)
            newcl->c.debugname = originalCl->c.debugname;

        for (int i = 0; i < originalCl->nupvalues; i++)
            setobj2n(L, &newcl->c.upvals[i], &originalCl->c.upvals[i]);

        newcl->c.f = originalCl->c.f;
        newcl->c.cont = originalCl->c.cont;
        setclvalue(L, L->top, newcl);
        L->top++;

        const auto executionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L);
        const auto envContext = executionEngine->GetEnvironmentContext();

        // Redirect original newcclosure to the cloned one.
        if (envContext->m_newcclosures.contains(lua_toclosure(L, 1)))
            envContext->m_newcclosures[lua_toclosure(L, -1)] = envContext->m_newcclosures[lua_toclosure(L, 1)];

        return 1;
    }

    int newcclosure_stub(lua_State *L) {
        const auto executionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L);
        const auto envContext = executionEngine->GetEnvironmentContext();

        const auto current = clvalue(L->ci->func);

        if (!envContext->m_newcclosures.contains(current))
            luaL_error(L, "cannot resolve call, the closure is not mapped internally to an original closure");

        auto wrapContext = envContext->m_newcclosures.at(current);

        const auto original = wrapContext.GetReferencedObject(L);

        if (!original.has_value())
            luaL_error(L, "cannot resolve call: invalid reference");

        const auto nargs = lua_gettop(L);

        L->top->value.p = original.value();
        L->top->tt = ::lua_Type::LUA_TFUNCTION;
        L->top++;

        lua_insert(L, 1);

        const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
            RbxStuOffsets::GetSingleton()->
            GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

        lua_pushcclosure(L, task_defer, nullptr, 0);
        lua_insert(L, 1);

        const auto status = static_cast<lua_Status>(lua_pcall(L, nargs + 1 /* func is an arg for defer */, LUA_MULTRET,
                                                              0));

        const auto nL = lua_tothread(L, -1);
        nL->namecall = L->namecall; // Preserve namecall

        executionEngine->YieldThread(L, [nL, L](const std::shared_ptr<YieldRequest> &yieldContext) {
            while (nL->isactive || (nL->status == LUA_YIELD)) {
                _mm_pause();
                std::this_thread::yield();
            }

            yieldContext->fpCompletionCallback = [nL, L]() {
                const bool success = nL->status == LUA_OK;
                if (success) {
                    luaC_threadbarrier(L);
                    luaC_threadbarrier(nL);
                    lua_xmove(nL, L, lua_gettop(nL));
                } else {
                    lua_xmove(nL, L, 1); // Move error message

                    const auto realError = Utilities::GetSingleton()->FromLuaErrorMessageToCErrorMessage(
                        lua_tostring(L, -1));
                    lua_pop(L, 1);
                    lua_pushstring(L, realError.c_str());
                }

                return YieldResult{success, lua_gettop(L), !success ? lua_tostring(L, -1) : nullptr};
            };

            yieldContext->bIsReady = true;
        }, false);

        return lua_yield(L, 0);
    }

    int Closures::newcclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        const auto debugName = luaL_optstring(L, 2, nullptr);

        lua_pushcclosure(L, newcclosure_stub, debugName, 0);

        const auto executionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L);
        const auto envContext = executionEngine->GetEnvironmentContext();

        const auto refId = lua_ref(L, 1);

        envContext->m_newcclosures[lua_toclosure(L, -1)] = {refId};

        return 1;
    }

    const luaL_Reg *Closures::GetFunctionRegistry() {
        static luaL_Reg closuresLib[] = {
            {"iscclosure", RbxStu::StuLuau::Environment::UNC::Closures::iscclosure},
            {"islclosure", RbxStu::StuLuau::Environment::UNC::Closures::islclosure},
            {"clonefunction", RbxStu::StuLuau::Environment::UNC::Closures::clonefunction},
            {"newcclosure", RbxStu::StuLuau::Environment::UNC::Closures::newcclosure},
            {nullptr, nullptr},
        };
        return closuresLib;
    }

    bool Closures::PushToGlobals() { return false; }

    const char *Closures::GetLibraryName() {
        return "closures";
    }
}
