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
#include "Luau/Compiler.h"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/LuauSecurity.hpp"

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

    int Closures::newcclosure_stub(lua_State *L) {
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

        // const auto task_defer = reinterpret_cast<RBX::Studio::FunctionTypes::task_defer>(
        //     RbxStuOffsets::GetSingleton()->
        //     GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer));

        // lua_pushcclosure(L, task_defer, nullptr, 0);
        // lua_insert(L, 1);

        L->baseCcalls++;

        const auto status = static_cast<lua_Status>(lua_pcall(L, nargs, LUA_MULTRET,
                                                              0));

        L->baseCcalls--;

        // const auto nL = lua_tothread(L, -1);
        // nL->namecall = L->namecall; // Preserve namecall

        /*
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
        */

        if (status != LUA_OK && status != LUA_YIELD) {
            const auto errorString = lua_tostring(L, -1);
            if (strcmp(errorString, "attempt to yield across metamethod/C-call boundary") == 0 || strcmp(
                    errorString, "thread is not yieldable") == 0)
                return lua_yield(L, 0);
        }

        if (status != LUA_OK) {
            const auto errorString = lua_tostring(L, -1);
            const auto realError = Utilities::GetSingleton()->FromLuaErrorMessageToCErrorMessage(errorString);

            lua_pushstring(L, realError.c_str());
            lua_error(L);
        }

        return lua_gettop(L);

        return lua_gettop(L);
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

    int Closures::newlclosure(lua_State *L) {
        luaL_checktype(L, -1, LUA_TFUNCTION);

        if (!lua_iscfunction(L, -1))
            lua_ref(L, -1);

        lua_newtable(L); // t
        lua_newtable(L); // Meta

        lua_pushvalue(L, LUA_GLOBALSINDEX);
        lua_setfield(L, -2, "__index");
        lua_setreadonly(L, -1, true);
        lua_setmetatable(L, -2);

        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "abcdefg"); // Set abcdefg to that of idx
        const auto code = "return abcdefg(...)";

        constexpr auto compileOpts = Luau::CompileOptions{0, 0};
        const auto bytecode = Luau::compile(code, compileOpts);
        luau_load(L, "=RbxStuV3_newlclosurewrapper", bytecode.c_str(), bytecode.size(), -1);

        lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
        return 1;
    }

    // ReSharper disable once CppDFAConstantFunctionResult
    int Closures::isourclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        const auto closure = lua_toclosure(L, 1);

        lua_pushboolean(L, LuauSecurity::GetSingleton()->IsOurClosure(closure));
        return 1;
    }

    int Closures::loadstring(lua_State *L) {
        const auto luauCode = luaL_checkstring(L, 1);
        const auto chunkName = luaL_optstring(L, 2, "=loadstring");
        constexpr auto compileOpts = Luau::CompileOptions{1, 2};
        const auto bytecode = Luau::compile(luauCode, compileOpts);

        if (luau_load(L, chunkName, bytecode.c_str(), bytecode.size(), 0) != lua_Status::LUA_OK) {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        LuauSecurity::GetSingleton()->ElevateClosure(
            lua_toclosure(L, -1), RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false); // env is not safe anymore.
        return 1;
    }

    const luaL_Reg *Closures::GetFunctionRegistry() {
        static luaL_Reg closuresLib[] = {
            {"isourclosure", RbxStu::StuLuau::Environment::UNC::Closures::isourclosure},

            {"iscclosure", RbxStu::StuLuau::Environment::UNC::Closures::iscclosure},
            {"islclosure", RbxStu::StuLuau::Environment::UNC::Closures::islclosure},

            {"clonefunction", RbxStu::StuLuau::Environment::UNC::Closures::clonefunction},

            {"loadstring", RbxStu::StuLuau::Environment::UNC::Closures::loadstring},
            {"newcclosure", RbxStu::StuLuau::Environment::UNC::Closures::newcclosure},
            {"newlclosure", RbxStu::StuLuau::Environment::UNC::Closures::newlclosure},
            {nullptr, nullptr},
        };
        return closuresLib;
    }

    bool Closures::PushToGlobals() { return true; }

    const char *Closures::GetLibraryName() {
        return "closures";
    }
}
