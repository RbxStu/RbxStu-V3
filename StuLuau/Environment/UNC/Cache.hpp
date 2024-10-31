//
// Created by Dottik on 31/10/2024.
//

#pragma once

#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Cache final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int invalidate(lua_State *L);

        static int replace(lua_State *L);

        static int iscached(lua_State *L);

        ~Cache() override = default;

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
}
