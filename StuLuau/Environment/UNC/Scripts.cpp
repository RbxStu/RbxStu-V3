//
// Created by Pixeluted on 01/11/2024.
//

#include "Scripts.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "StuLuau/LuauSecurity.hpp"

namespace RbxStu::StuLuau::Environment::UNC {

    int Scripts::setidentity(lua_State *L) {
        luaL_checkinteger(L, 1);
        if (lua_gettop(L) > 1)
            luaL_errorL(L, "You only need to provide one argument!");

        const auto desiredIdentity = lua_tointeger(L, 1);
        const auto luauSecurity = LuauSecurity::GetSingleton();
        const auto desiredExecutionContext = luauSecurity->GetExecutionSecurityFromIdentity(desiredIdentity);

        luauSecurity->SetThreadSecurity(L, desiredExecutionContext, true);

        Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(L)->YieldThread(
                L, [](const std::shared_ptr<RbxStu::StuLuau::YieldRequest> &yieldRequest) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    yieldRequest->fpCompletionCallback = []() {
                        return YieldResult{true, 0};
                    };
                    yieldRequest->bIsReady = true;
                }, true);

        return lua_yield(L, 0);
    }

    int Scripts::getidentity(lua_State *L) {
        if (lua_gettop(L) > 0)
            luaL_errorL(L, "You don't need to provide any arguments!");

        const int32_t currentIdentity = LuauSecurity::GetThreadExtraspace(L)->contextInformation.identity;
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

                {"getidentity", Scripts::getidentity},
                {"getthreadcontext", Scripts::getidentity},


                {nullptr, nullptr}};

        return slib;
    }

}
