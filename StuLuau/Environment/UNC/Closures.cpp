//
// Created by Dottik on 25/10/2024.
//

#include "Closures.hpp"

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

    luaL_Reg *Closures::GetFunctionRegistry() {
        return new luaL_Reg [0x5]{
            {"iscclosure", RbxStu::StuLuau::Environment::UNC::Closures::iscclosure},
            {"islclosure", RbxStu::StuLuau::Environment::UNC::Closures::islclosure},
            {nullptr, nullptr},
        };
    }

    const char *Closures::GetLibraryName() {
        return "closures";
    }
}
