//
// Created by Dottik on 30/10/2024.
//

#include "NewGlobals.hpp"

#include "lobject.h"

namespace RbxStu::StuLuau::Environment::Custom {
    int NewGlobals::setuntouched(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        luaL_checktype(L, 2, lua_Type::LUA_TBOOLEAN);

        lua_setsafeenv(L, 1, lua_toboolean(L, 2));
        return 0;
    }


    int NewGlobals::isuntouched(lua_State *L) {
        luaL_checktype(L, 1, lua_Type::LUA_TTABLE);
        lua_pushboolean(L, static_cast<const Table *>(lua_topointer(L, 1))->safeenv);

        return 1;
    }

    const luaL_Reg *NewGlobals::GetFunctionRegistry() {
        static luaL_Reg functions[] = {
            {"setuntouched", NewGlobals::setuntouched},
            {"isuntouched", NewGlobals::isuntouched},
            {nullptr, nullptr}
        };
        return functions;
    }

    bool NewGlobals::PushToGlobals() {
        return false;
    }

    const char * NewGlobals::GetLibraryName() {
        return "rbxstu";
    }
}
