//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Closures final : public RbxStu::StuLuau::Environment::Library {
    public:
        ~Closures() override = default;

        static int iscclosure(lua_State *L);

        static int islclosure(lua_State *L);

        static int clonefunction(lua_State *L);

        static int newcclosure_stub(lua_State *L);

        static int newcclosure_stubcont(lua_State *L, int status);

        static int newcclosure(lua_State *L);

        static int newlclosure(lua_State *L);

        static int isourclosure(lua_State *L);

        static int loadstring(lua_State *L);

        static int isunhookable(lua_State *L);

        static int makeunhookable(lua_State *L);

        static int hookfunction(lua_State *L);

        static int ishooked(lua_State *L);

        static int restorefunction(lua_State *L);

        static int hookmetamethod(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
} // RbxStu::StuLuau::Environment::UNC
