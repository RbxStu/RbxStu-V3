//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Globals final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int getgenv(lua_State *L);

        static int getrenv(lua_State *L);

        static int gettenv(lua_State *L);

        static int httpget(lua_State *L);

        ~Globals() override = default;

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
} // RbxStu::StuLuau::Environment::UNC
