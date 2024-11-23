//
// Created by Dottik on 2/11/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Crypt final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int base64encode(lua_State *L);

        static int base64decode(lua_State *L);

        static int generatebytes(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override { return true; }

        const char *GetLibraryName() override { return "crypt"; }
    };
}
