//
// Created by Dottik on 27/10/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::Custom {
    class Memory final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int getgc(lua_State *L);

        static int reference_object(lua_State *L);

        static int unreference_object(lua_State *L);

        ~Memory() override = default;

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;;

        const char *GetLibraryName() override;
    };
}
