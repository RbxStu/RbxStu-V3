//
// Created by Dottik on 30/10/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::Custom {
    class NewGlobals final : public RbxStu::StuLuau::Environment::Library {
    public:
        ~NewGlobals() override = default;

        static int setuntouched(lua_State *L);

        static int isuntouched(lua_State *L);

        static int getcapabilities(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;;

        bool PushToGlobals() override;;

        const char *GetLibraryName() override;;
    };
}
