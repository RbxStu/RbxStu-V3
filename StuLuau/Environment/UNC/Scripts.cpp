//
// Created by Pixeluted on 01/11/2024.
//

#include "Scripts.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "StuLuau/LuauSecurity.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Scripts::setidentity(lua_State *L) {
        luaL_checkinteger(L, 1);

        const auto identity = lua_tointeger(L, 1);

        if (identity < 0 || identity > 8)
            luaL_error(L, "cannot set the thread's identity to invalid values.");

        const auto luauSecurity = LuauSecurity::GetSingleton();
        const auto desiredExecutionContext = luauSecurity->GetExecutionSecurityFromIdentity(identity);

        luauSecurity->SetThreadSecurity(L, desiredExecutionContext, identity, luauSecurity->IsMarkedThread(L));

        Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L)->YieldThread(
            L, [](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
                yieldRequest->fpCompletionCallback = []() {
                    return YieldResult{true, 0};
                };
                yieldRequest->bIsReady = true;
            }, true);

        return lua_yield(L, 0);
    }

    int Scripts::getidentity(lua_State *L) {
        const int32_t currentIdentity = GetThreadExtraspace(L)->contextInformation.identity;
        lua_pushinteger(L, currentIdentity);
        return 1;
    }


    const char *Scripts::GetLibraryName() {
        return "Scripts";
    }

    bool Scripts::PushToGlobals() {
        return true;
    }

    const luaL_Reg *Scripts::GetFunctionRegistry() {
        static luaL_Reg slib[] = {
            {"setidentity", Scripts::setidentity},
            {"setthreadcontext", Scripts::setidentity},
            {"setthreadidentity", Scripts::setidentity},

            {"getidentity", Scripts::getidentity},
            {"getthreadidentity", Scripts::getidentity},
            {"getthreadcontext", Scripts::getidentity},


            {nullptr, nullptr}
        };

        return slib;
    }
}
