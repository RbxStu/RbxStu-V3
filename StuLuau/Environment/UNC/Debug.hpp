//
// Created by Dottik on 22/11/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Debug final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int getconstants(lua_State *L);

        static int getconstant(lua_State *L);

        static int setconstant(lua_State *L);

        static int getinfo(lua_State *L);

        static int getproto(lua_State *L);

        static int getprotos(lua_State *L);

        static int setstack(lua_State *L);

        static int getstack(lua_State *L);

        static int lsetupvalue(lua_State *L);

        static int getupvalue(lua_State *L);

        static int getupvalues(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
}
