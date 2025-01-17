//
// Created by Dottik on 30/10/2024.
//

#include "NewGlobals.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "StuLuau/Extensions/luauext.hpp"
#include "StuLuau/LuauSecurity.hpp"
#include "lobject.h"

namespace RbxStu::StuLuau::Environment::Custom {
    int NewGlobals::setuntouched(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        luaL_checktype(L, 2, lua_Type::LUA_TBOOLEAN);
        lua_normalisestack(L, 2);

        lua_setsafeenv(L, 1, lua_toboolean(L, 2));
        return 0;
    }


    int NewGlobals::isuntouched(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        lua_normalisestack(L, 1);
        lua_preparepush(L, 1);
        lua_pushboolean(L, static_cast<const Table *>(lua_topointer(L, 1))->safeenv);
        return 1;
    }

    int NewGlobals::getcapabilities(lua_State *L) {
        // Force hex format.
        lua_normalisestack(L, 0);
        lua_preparepushcollectable(L, 1);
        lua_pushstring(L, std::format("{}", reinterpret_cast<void *>(GetThreadExtraspace(L)->capabilities)).c_str());
        return 1;
    }

    int NewGlobals::getobjectaddress(lua_State *L) {
        lua_normalisestack(L, 1);
        lua_preparepushcollectable(L, 1);
        lua_pushstring(L, std::format("{}", lua_topointer(L, 1)).c_str());
        return 1;
    }

    int NewGlobals::getdatamodeltype(lua_State *L) {
        const auto executionEngine =
                RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->GetExecutionEngine(
                        L);
        lua_normalisestack(L, 0);
        lua_preparepush(L, 1);
        lua_pushinteger(L, executionEngine->GetDataModelType());
        return 1;
    }

    int NewGlobals::getuserdatatag(lua_State *L) {
        luaL_checktype(L, 1, LUA_TUSERDATA);
        lua_normalisestack(L, 1);
        lua_preparepush(L, 1);
        lua_pushinteger(L, lua_userdatatag(L, 1));
        return 1;
    }

    const luaL_Reg *NewGlobals::GetFunctionRegistry() {
        static luaL_Reg functions[] = {{"setuntouched", NewGlobals::setuntouched},
                                       {"isuntouched", NewGlobals::isuntouched},
                                       {"getcapabilities", NewGlobals::getcapabilities},
                                       {"getobjectaddress", NewGlobals::getobjectaddress},
                                       {"getdatamodeltype", NewGlobals::getdatamodeltype},
                                       {"getuserdatatag", NewGlobals::getuserdatatag},

                                       {nullptr, nullptr}};
        return functions;
    }

    bool NewGlobals::PushToGlobals() { return true; }

    const char *NewGlobals::GetLibraryName() { return "rbxstu"; }
} // namespace RbxStu::StuLuau::Environment::Custom
