//
// Created by Dottik on 30/10/2024.
//

#include "NewGlobals.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "lobject.h"
#include "StuLuau/LuauSecurity.hpp"

namespace RbxStu::StuLuau::Environment::Custom {
    int NewGlobals::setuntouched(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        luaL_checktype(L, 2, lua_Type::LUA_TBOOLEAN);

        lua_setsafeenv(L, 1, lua_toboolean(L, 2));
        return 0;
    }


    int NewGlobals::isuntouched(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        lua_rawcheckstack(L, 1);
        lua_pushboolean(L, static_cast<const Table *>(lua_topointer(L, 1))->safeenv);
        return 1;
    }

    int NewGlobals::getcapabilities(lua_State *L) {
        // Force hex format.
        lua_rawcheckstack(L, 1);
        lua_pushstring(
            L, std::format("{}", reinterpret_cast<void *>(LuauSecurity::GetThreadExtraspace(L)->capabilities)).c_str());
        return 1;
    }

    int NewGlobals::getobjectaddress(lua_State *L) {
        lua_rawcheckstack(L, 1);
        lua_pushstring(L, std::format("{}", lua_topointer(L, 1)).c_str());

        return 1;
    }

    int NewGlobals::getdatamodeltype(lua_State *L) {
        const auto executionEngine = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->
                GetExecutionEngine(L);
        lua_pushinteger(L, executionEngine->GetDataModelType());
        return 1;
    }

    const luaL_Reg *NewGlobals::GetFunctionRegistry() {
        static luaL_Reg functions[] = {
            {"setuntouched", NewGlobals::setuntouched},
            {"isuntouched", NewGlobals::isuntouched},
            {"getcapabilities", NewGlobals::getcapabilities},
            {"getobjectaddress", NewGlobals::getobjectaddress},
            {"getdatamodeltype", NewGlobals::getdatamodeltype},
            {nullptr, nullptr}
        };
        return functions;
    }

    bool NewGlobals::PushToGlobals() {
        return true;
    }

    const char *NewGlobals::GetLibraryName() {
        return "rbxstu";
    }
}
