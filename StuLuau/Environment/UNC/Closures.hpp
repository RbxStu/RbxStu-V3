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

        const luaL_Reg *GetFunctionRegistry() override;

        const char *GetLibraryName() override;;
    };
} // RbxStu::StuLuau::Environment::UNC
