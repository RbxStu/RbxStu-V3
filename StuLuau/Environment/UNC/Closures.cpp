//
// Created by Dottik on 25/10/2024.
//

#include "Closures.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <Utilities.hpp>

#include "lapi.h"
#include "lfunc.h"
#include "lgc.h"

#include "Luau/Compiler.h"
#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/Extensions/luauext.hpp"
#include "StuLuau/LuauSecurity.hpp"
#include "lobject.h"
#include "lstate.h"

namespace RbxStu::StuLuau::Environment::UNC {
    int Closures::iscclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        lua_pushboolean(L, lua_iscfunction(L, 1));
        return 1;
    }

    int Closures::islclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        lua_pushboolean(L, !lua_iscfunction(L, 1));
        return 1;
    }

    int Closures::clonefunction(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        lua_preparepush(L, 1);
        if (!lua_iscfunction(L, 1)) {
            lua_clonefunction(L, 1);
            return 1;
        }

        const auto originalCl = lua_tomutclosure(L, 1);

        Closure *newcl = luaF_newCclosure(L, originalCl->nupvalues, originalCl->env);

        if (originalCl->c.debugname != nullptr)
            newcl->c.debugname = originalCl->c.debugname;

        for (int i = 0; i < originalCl->nupvalues; i++)
            setobj2n(L, &newcl->c.upvals[i], &originalCl->c.upvals[i]);

        newcl->c.f = originalCl->c.f;
        newcl->c.cont = originalCl->c.cont;
        lua_preparepushcollectable(L, 1);
        setclvalue(L, L->top, newcl);
        L->top++;

        const auto executionEngine =
                RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(
                        L);
        const auto envContext = executionEngine->GetEnvironmentContext();

        // Redirect original newcclosure to the cloned one.
        if (envContext->m_newcclosures.contains(lua_tomutclosure(L, 1)))
            envContext->m_newcclosures[lua_tomutclosure(L, -1)] = envContext->m_newcclosures.at(lua_tomutclosure(L, 1));

        if (envContext->IsDataModelMetamethod(lua_tomutclosure(L, 1)))
            envContext->DefineNewDataModelMetaMethodClosure(lua_tomutclosure(L, 1), lua_tomutclosure(L, -1));

        return 1;
    }

    int Closures::newcclosure_stub(lua_State *L) {
        const auto executionEngine =
                RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(
                        L);
        const auto envContext = executionEngine->GetEnvironmentContext();

        const auto current = clvalue(L->ci->func);

        if (!envContext->m_newcclosures.contains(current))
            luaL_error(L, "cannot resolve call, the closure is not mapped internally to an original closure");

        auto wrapContext = envContext->m_newcclosures.at(current);

        const auto original = wrapContext.GetReferencedObject(L);

        if (!original.has_value())
            luaL_error(L, "cannot resolve call: invalid reference");

        const auto nargs = lua_gettop(L);

        lua_preparepushcollectable(L, 1);
        L->top->value.p = original.value();
        L->top->tt = ::lua_Type::LUA_TFUNCTION;
        L->top++;

        lua_insert(L, 1);

        L->ci->flags |= LUA_CALLINFO_HANDLE;

        L->baseCcalls++;

        const auto status = static_cast<lua_Status>(lua_pcall(L, nargs, LUA_MULTRET, 0));
        L->baseCcalls--;

        if (status == LUA_OK && (L->status == LUA_YIELD || L->status == LUA_BREAK))
            return lua_yield(L, 0);

        if (status != LUA_OK && status != LUA_YIELD) {
            const auto errorString = lua_tostring(L, -1);
            if (strcmp(errorString, "attempt to yield across metamethod/C-call boundary") == 0)
                return lua_yield(L, 0);
        }

        if (status != LUA_OK) {
            const auto errorString = lua_tostring(L, -1);
            const auto realError = Utilities::GetSingleton()->FromLuaErrorMessageToCErrorMessage(errorString);

            lua_preparepushcollectable(L, 1);
            lua_pushstring(L, realError.c_str());
            lua_error(L);
        }

        return lua_gettop(L);
    }

    int Closures::newcclosure_stubcont(lua_State *L, int status) {
        if (status == LUA_OK)
            return lua_gettop(L); // Success, ret all results.

        // Failure, we must clear the error message on the stack top and re-push it.
        lua_preparepushcollectable(L, 1);

        const auto errorString = lua_tostring(L, -1);
        const auto realError = Utilities::GetSingleton()->FromLuaErrorMessageToCErrorMessage(errorString);

        lua_pushstring(L, realError.c_str());
        lua_error(L);
    }

    int Closures::newcclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        const auto debugName = luaL_optstring(L, 2, nullptr);
        lua_normalisestack(L, 1);
        lua_preparepushcollectable(L, 1);
        lua_pushcclosurek(L, newcclosure_stub, debugName, 0, newcclosure_stubcont);

        const auto executionEngine =
                RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(
                        L);
        const auto envContext = executionEngine->GetEnvironmentContext();

        const auto refId = lua_ref(L, 1);

        envContext->m_newcclosures[lua_tomutclosure(L, -1)] =
                ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION>{refId};

        return 1;
    }

    int Closures::newlclosure(lua_State *L) {
        luaL_checktype(L, -1, LUA_TFUNCTION);

        if (!lua_iscfunction(L, -1))
            lua_ref(L, -1);

        lua_preparepushcollectable(L, 3);
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

        LuauSecurity::GetSingleton()->ElevateClosure(lua_tomutclosure(L, -1),
                                                     RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);

        lua_remove(L, lua_gettop(L) - 1); // Balance lua stack.
        return 1;
    }

    // ReSharper disable once CppDFAConstantFunctionResult
    int Closures::isourclosure(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        const auto closure = lua_tomutclosure(L, 1);
        lua_preparepush(L, 1);
        lua_pushboolean(L, LuauSecurity::GetSingleton()->IsOurClosure(closure));
        return 1;
    }

    int Closures::loadstring(lua_State *L) {
        const auto luauCode = luaL_checkstring(L, 1);
        const auto chunkName = luaL_optstring(L, 2, "=loadstring");
        lua_normalisestack(L, 1);
        lua_preparepush(L, 1);
        constexpr auto compileOpts = Luau::CompileOptions{1, 2};
        const auto bytecode = Luau::compile(luauCode, compileOpts);

        if (luau_load(L, chunkName, bytecode.c_str(), bytecode.size(), 0) != lua_Status::LUA_OK) {
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            return 2;
        }

        LuauSecurity::GetSingleton()->ElevateClosure(lua_tomutclosure(L, -1),
                                                     RbxStu::StuLuau::ExecutionSecurity::RobloxExecutor);

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false); // env is not safe anymore.
        return 1;
    }

    int Closures::isunhookable(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        lua_preparepush(L, 1);
        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);
        const auto environmentContext = executionEngine->GetEnvironmentContext();

        lua_pushboolean(L, environmentContext->IsUnhookable(lua_tomutclosure(L, 1)));

        return 1;
    }

    int Closures::makeunhookable(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);

        if (const auto environmentContext = executionEngine->GetEnvironmentContext();
            !environmentContext->IsUnhookable(lua_tomutclosure(L, 1)))
            environmentContext->MakeUnhookable(lua_tomutclosure(L, 1));

        return 0;
    }

    int Closures::hookfunction(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        luaL_checktype(L, 2, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 2);

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Hooking function");

        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);
        const auto environmentContext = executionEngine->GetEnvironmentContext();

        if (environmentContext->IsUnhookable(lua_tomutclosure(L, 1)))
            luaL_argerror(L, 1, "this function cannot be hooked from Luau");

        lua_preparepushcollectable(L, 2);
        lua_pushcclosure(L, clonefunction, nullptr, 0);
        lua_pushvalue(L, 1);
        lua_pcall(L, 1, 1, 0);
        const auto originalCloned = lua_tomutclosure(L, -1);

        lua_preparepushcollectable(L, 2);
        lua_pushcclosure(L, clonefunction, nullptr, 0);
        lua_pushvalue(L, 1);
        lua_pcall(L, 1, 1, 0);


        // Luau closures must be referenced, else they will get collected.
        const auto hookedWithRef = lua_ref(L, 2);
        if (!environmentContext->m_functionHooks.contains(lua_tomutclosure(L, 1))) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Created Hook Information");
            environmentContext->m_functionHooks[lua_tomutclosure(L, 1)] =
                    HookInformation{ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION>{lua_ref(L, -1)},
                                    ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION>{hookedWithRef}};
        } else {
            // We DO NOT want to modify an already defined function hook, if so, we will be unable to restore it to its
            // original definition! This means ORIGINAL must NEVER be modified on hookfunction other than when the
            // function hook is originally and initially defined.
        }

        lua_pop(L, 2);

        /*
         *  For the purposes of being able to hook all kinds of closures, we want to be able to hook EVERYTHING to
         * EVERYTHING. the easiest solution, is to wrap the other closure into what the first closure is, allowing to
         * hook everything to everything, without upvalue limits, at the small cost of memory.
         */

        /*
         *  NEWCCLOSURE -> ANY
         *  C CLOSURE -> ANY
         */
        if (lua_iscfunction(L, 1)) {
            if (!lua_iscfunction(L, 2) || !environmentContext->IsWrappedClosure(lua_tomutclosure(L, 2))) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Automatically wrapping closure into NEWCCLOSURE");
                // This function is a Luau/C function, it is simpler to wrap this Luau/C closure into a C closure using
                // newcclosure before proceeding and replace it with the one in the lua state, this allows us to hook
                // without up value limits.
                lua_preparepushcollectable(L, 2);
                lua_pushcclosure(L, newcclosure, nullptr, 0);
                lua_pushvalue(L, 2);
                lua_call(L, 1, 1);
            } else {
                lua_preparepush(L, 1);
                lua_pushvalue(L, 2);
            }

            // C function.
            const auto bIsHookTargetWrapped = environmentContext->IsWrappedClosure(lua_tomutclosure(L, 1));

            // NEWCCLOSURE -> ANY
            if (bIsHookTargetWrapped) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "NEWCCLOSURE->ANY");
                // Both closures are newcclosure closures, this means that hooking them is a simple pointer change.
                const auto hookWhatWrapped = environmentContext->m_newcclosures.at(lua_tomutclosure(L, 1));
                const auto hookWithWrapped = environmentContext->m_newcclosures.at(lua_tomutclosure(L, -1));

                auto hkInfo = environmentContext->m_functionHooks[lua_tomutclosure(L, 1)];
                hkInfo.hookedWith = hookWithWrapped;
                hkInfo.dwHookedType = FunctionKind::NewCClosure;
                hkInfo.dwHookWithType = FunctionKind::NewCClosure;
                environmentContext->m_functionHooks[lua_tomutclosure(L, 1)] = hkInfo;
                environmentContext->m_newcclosures[lua_tomutclosure(L, 1)] = hookWithWrapped;
                lua_preparepushcollectable(L, 1);
                L->top->value.p = originalCloned;
                L->top->tt = LUA_TFUNCTION;
                L->top++;
                environmentContext->m_newcclosures[lua_tomutclosure(L, -1)] = hookWhatWrapped;
                // Point the cloned one to the correct original.
                return 1;
            }

            // ReSharper disable once CppDFAConstantConditions
            if (!bIsHookTargetWrapped) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "C->ANY");
                // C -> ANY
                const auto hookWithWrapped = environmentContext->m_newcclosures.at(lua_tomutclosure(L, -1));

                auto hkInfo = environmentContext->m_functionHooks[lua_tomutclosure(L, 1)];
                // original needn't be modified.
                hkInfo.hookedWith = hookWithWrapped;
                hkInfo.dwHookedType = FunctionKind::CClosure;
                hkInfo.dwHookWithType = FunctionKind::NewCClosure;
                environmentContext->m_functionHooks[lua_tomutclosure(L, 1)] = hkInfo;

                const auto hookWhat = lua_tomutclosure(L, 1);
                const auto hookWith = lua_tomutclosure(L, -1);
                // Setting upvalues is not required here, as newcclosure stub doers not use them.
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Replacing native frames");

                hookWhat->c.f = [](lua_State *L) { return 0; };
                hookWhat->c.cont = [](lua_State *L, int status) { return 0; };
                hookWhat->env = hookWith->env;
                hookWhat->stacksize = hookWith->stacksize;
                hookWhat->preload = hookWith->preload;

                for (int i = 0; i < hookWith->nupvalues; i++)
                    setobj2n(L, &hookWhat->c.upvals[i], &hookWith->c.upvals[i]);

                hookWhat->nupvalues = hookWith->nupvalues;
                hookWhat->c.f = hookWith->c.f;
                hookWhat->c.cont = hookWith->c.cont;

                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "NEWCCLOSURE SET");

                environmentContext->m_newcclosures[hookWhat] =
                        ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION>{lua_ref(L, -1)};

                lua_preparepushcollectable(L, 1);
                L->top->value.p = originalCloned;
                L->top->tt = LUA_TFUNCTION;
                L->top++;

                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "RETURNING ORIGINAL");
                return 1;
            }
        }

        if (!lua_iscfunction(L, 1)) {
            auto wrapped = false;
            if (lua_iscfunction(L, 2) || (lua_tomutclosure(L, 2)->nupvalues > lua_tomutclosure(L, 1)->nupvalues)) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Wrapping ANY to Luau Closure");
                // This function is a C or has more upvalues than the target function, it is simpler to wrap this C
                // closure into a Luau closure using newlclosure before proceeding and replace it with the one in the
                // lua state, this allows us to hook without up value limits, which is also another pro of it lmao.
                lua_preparepushcollectable(L, 2);
                lua_pushcclosure(L, newlclosure, nullptr, 0);
                lua_pushvalue(L, 2);
                lua_call(L, 1, 1);
                wrapped = true;
            }

            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Anonymous, "Luau->ANY");

            /*
             *  Luau -> ANY
             */
            const auto hookWhat = lua_tomutclosure(L, 1);
            const auto hookWith = lua_tomutclosure(L, wrapped ? -1 : 2);

            hookWhat->env = hookWith->env;
            hookWhat->stacksize = hookWith->stacksize;
            hookWhat->preload = hookWith->preload;

            for (int i = 0; i < hookWith->nupvalues; i++)
                setobj2n(L, &hookWhat->l.uprefs[i], &hookWith->l.uprefs[i]);

            hookWhat->nupvalues = hookWith->nupvalues;
            hookWhat->l.p = hookWith->l.p;

            auto hkInfo = environmentContext->m_functionHooks[lua_tomutclosure(L, 1)];
            // original needn't be modified.
            hkInfo.hookedWith =
                    ReferencedLuauObject<Closure *, ::lua_Type::LUA_TFUNCTION>(lua_ref(L, wrapped ? -1 : 2));
            hkInfo.dwHookedType = FunctionKind::LuauClosure;
            hkInfo.dwHookWithType = FunctionKind::LuauClosure;
            environmentContext->m_functionHooks[lua_tomutclosure(L, 1)] = hkInfo;

            lua_preparepushcollectable(L, 1);
            L->top->value.p = originalCloned;
            L->top->tt = LUA_TFUNCTION;
            L->top++;
            return 1;
        }

        luaL_error(L, "hookfunction: not supported");
        return 0;
    }

    int Closures::ishooked(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        lua_preparepush(L, 1);
        const auto whatClosure = lua_tomutclosure(L, 1);

        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);
        const auto environmentContext = executionEngine->GetEnvironmentContext();

        lua_pushboolean(L, environmentContext->m_functionHooks.contains(whatClosure));

        return 1;
    }

    int Closures::restorefunction(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 1);
        const auto unhookWhat = lua_tomutclosure(L, 1);

        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);
        const auto environmentContext = executionEngine->GetEnvironmentContext();

        if (!environmentContext->m_functionHooks.contains(unhookWhat))
            return 0; // Function was not hooked.

        auto hookInfo = environmentContext->m_functionHooks[unhookWhat];

        if (!hookInfo.original.GetReferencedObject(L).has_value())
            luaL_error(L, "Cannot obtain original function to unhook.");

        if (hookInfo.dwHookedType == hookInfo.dwHookWithType) {
            // The hook in this case is a simple replace back to original.
            if (hookInfo.original.GetReferencedObject(L).has_value()) {
                // Original is alive.
                if (hookInfo.original.GetReferencedObject(L).value()->isC) {
                    // C-type unhook.
                    if (hookInfo.dwHookedType == FunctionKind::NewCClosure) {
                        // newcclosure -> newcclosure hooks mostly go off the grounds of newcclosure index moving.
                        environmentContext->m_newcclosures[unhookWhat] = hookInfo.original;
                        hookInfo.hookedWith.UnreferenceObject(L);
                    } else {
                        const auto unhookWith = hookInfo.original.GetReferencedObject(L).value();
                        unhookWhat->c.f = [](lua_State *L) { return 0; };
                        unhookWhat->c.cont = [](lua_State *L, int status) { return 0; };
                        unhookWhat->env = unhookWith->env;
                        unhookWhat->stacksize = unhookWith->stacksize;
                        unhookWhat->preload = unhookWith->preload;

                        for (int i = 0; i < unhookWith->nupvalues; i++)
                            setobj2n(L, &unhookWhat->c.upvals[i], &unhookWith->c.upvals[i]);

                        unhookWhat->nupvalues = unhookWith->nupvalues;
                        unhookWhat->c.f = unhookWith->c.f;
                        unhookWhat->c.cont = unhookWith->c.cont;

                        hookInfo.original.UnreferenceObject(L);
                        hookInfo.hookedWith.UnreferenceObject(L);
                    }
                } else {
                    // Luau->Luau unhook.
                    const auto unhookWith = hookInfo.original.GetReferencedObject(L).value();
                    unhookWhat->env = unhookWith->env;
                    unhookWhat->stacksize = unhookWith->stacksize;
                    unhookWhat->preload = unhookWith->preload;

                    for (int i = 0; i < unhookWith->nupvalues; i++)
                        setobj2n(L, &unhookWhat->l.uprefs[i], &unhookWith->l.uprefs[i]);

                    unhookWhat->nupvalues = unhookWith->nupvalues;
                    unhookWhat->l.p = unhookWith->l.p;

                    hookInfo.original.UnreferenceObject(L);
                    hookInfo.hookedWith.UnreferenceObject(L);
                }
            }
        }

        if (hookInfo.dwHookedType == FunctionKind::CClosure && hookInfo.dwHookWithType == FunctionKind::NewCClosure) {
            // Wrapped C closure unhook.
            if (environmentContext->m_newcclosures.contains(unhookWhat))
                environmentContext->m_newcclosures.erase(unhookWhat); // Erase.

            const auto originalFunction = hookInfo.original.GetReferencedObject(L).value();
            unhookWhat->c.f = [](lua_State *L) { return 0; };
            unhookWhat->c.cont = [](lua_State *L, int status) { return 0; };
            unhookWhat->env = originalFunction->env;
            unhookWhat->stacksize = originalFunction->stacksize;
            unhookWhat->preload = originalFunction->preload;

            for (int i = 0; i < originalFunction->nupvalues; i++)
                setobj2n(L, &unhookWhat->c.upvals[i], &originalFunction->c.upvals[i]);

            unhookWhat->nupvalues = originalFunction->nupvalues;
            unhookWhat->c.f = originalFunction->c.f;
            unhookWhat->c.cont = originalFunction->c.cont;

            hookInfo.original.UnreferenceObject(L);
            hookInfo.hookedWith.UnreferenceObject(L);
        }


        if (environmentContext->m_functionHooks.contains(unhookWhat))
            environmentContext->m_functionHooks.erase(unhookWhat);

        return 0;
    }

    int Closures::hookmetamethod(lua_State *L) {
        luaL_checkany(L, 1);
        const auto szMetaFieldName = luaL_checkstring(L, 2);
        luaL_checktype(L, 3, ::lua_Type::LUA_TFUNCTION);
        lua_normalisestack(L, 3);

        const auto executionEngine =
                Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L);
        const auto environmentContext = executionEngine->GetEnvironmentContext();

        if (lua_topointer(L, 1) == executionEngine->GetInitializationInformation()->executorState->gt)
            luaL_argerror(L, 1, "cannot hookmetamethod on the global environment of the executor");

        auto found = false;
        for (auto i = luaT_eventname; i < luaT_eventname + 21; i++) {
            // 21 == sizeof(luaT_eventname)
            if (strcmp(*i, szMetaFieldName) == 0) {
                found = true;
                break;
            }
        }

        if (!found)
            luaL_argerror(L, 2, std::format("invalid metafield '{}'", szMetaFieldName).c_str());

        if (!lua_getmetatable(L, 1)) {
            luaL_argerror(L, 1, "object has no metatable");
        } else {
            lua_setreadonly(L, -1, false);
            lua_pop(L, 1);
        }

        lua_preparepushcollectable(L, 1);
        if (!luaL_getmetafield(L, 1, szMetaFieldName))
            luaL_argerror(
                    L, 1,
                    "cannot hookmetamethod on an object with no metatable, or that its metafield does not exist.");

        if (!lua_isfunction(L, -1))
            luaL_argerror(L, 2,
                          "the metafield in the objects' metatable is not a function, and thus, cannot be hooked, use "
                          "getrawmetatable and override it instead.");

        /*
         *  For an arg guard we must push a stub closure that will call the given to us, as the one given to us is a lua
         * closure (most likely, and if it is not, we check if it is a newcclosure, if it is we will check as well, if
         * it is all C, however, we needn't check).
         */

        // if (lua_iscfunction(L, 3) && !environmentContext->IsWrappedClosure(lua_toclosure(L, 3))) {
        //     // C, no wrap.
        // }

        lua_preparepushcollectable(L, 3);
        lua_pushcclosure(L, hookfunction, nullptr, 0);
        lua_pushvalue(L, -2); // hookWhat
        lua_pushvalue(L, 3); // hookWith
        lua_call(L, 2, 1);

        lua_preparepushcollectable(L, 1);
        lua_getmetatable(L, 1);
        lua_setreadonly(L, -1, true);
        lua_pop(L, 1);

        return 1;
    }

    const luaL_Reg *Closures::GetFunctionRegistry() {
        static luaL_Reg closuresLib[] = {
                {"isunhookable", RbxStu::StuLuau::Environment::UNC::Closures::isunhookable},
                {"makeunhookable", RbxStu::StuLuau::Environment::UNC::Closures::makeunhookable},
                {"ishooked", RbxStu::StuLuau::Environment::UNC::Closures::ishooked},
                {"restorefunction", RbxStu::StuLuau::Environment::UNC::Closures::restorefunction},
                {"hookfunction", RbxStu::StuLuau::Environment::UNC::Closures::hookfunction},
                {"hookmetamethod", RbxStu::StuLuau::Environment::UNC::Closures::hookmetamethod},

                {"isourclosure", RbxStu::StuLuau::Environment::UNC::Closures::isourclosure},
                {"isexecutorclosure", RbxStu::StuLuau::Environment::UNC::Closures::isourclosure},

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

    const char *Closures::GetLibraryName() { return "closures"; }
} // namespace RbxStu::StuLuau::Environment::UNC
