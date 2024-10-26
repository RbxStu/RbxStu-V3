//
// Created by Dottik on 25/10/2024.
//

#include "Globals.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Scheduling/Job/InitializeExecutionEngineJob.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Globals::getgenv(lua_State *L) {
        const auto mainState = Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L)->GetInitializationInformation()->executorState;

        lua_pushvalue(mainState, LUA_GLOBALSINDEX);
        lua_xmove(mainState, L, 1);

        return 1;
    }

    int Globals::getrenv(lua_State *L) {
        const auto rL = lua_mainthread(Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
            GetExecutionEngine(L)->GetInitializationInformation()->globalState);

        lua_pushvalue(rL, LUA_GLOBALSINDEX);
        lua_xmove(rL, L, 1);

        return 1;
    }

    int Globals::gettenv(lua_State *L) {
        luaL_checktype(L, 1, ::lua_Type::LUA_TTHREAD);
        lua_settop(L, 1);
        const auto th = lua_tothread(L, 1);

        if (th->gt == nullptr) {
            lua_pushnil(L);
            return 1;
        }

        lua_pushvalue(th, LUA_GLOBALSINDEX);
        lua_xmove(th, L, 1);
        return 1;
    }


    const luaL_Reg *Globals::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {
            {"getrenv", RbxStu::StuLuau::Environment::UNC::Globals::getrenv},
            {"getgenv", RbxStu::StuLuau::Environment::UNC::Globals::getgenv},
            {"gettenv", RbxStu::StuLuau::Environment::UNC::Globals::gettenv},
            {nullptr, nullptr}
        };

        return libreg;
    }

    bool Globals::PushToGlobals() { return true; }

    const char *Globals::GetLibraryName() { return "rbxstu"; }
}
