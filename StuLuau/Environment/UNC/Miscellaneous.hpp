//
// Created by Dottik on 26/11/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Miscellaneous final : public RbxStu::StuLuau::Environment::Library {
    public:
        ~Miscellaneous() override = default;

        static int isparallel(lua_State *L);

        static int rbxcrash(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override { return true; }

        bool PushNoTable() override { return true; }

        const char *GetLibraryName() override { return nullptr; }
    };
}
