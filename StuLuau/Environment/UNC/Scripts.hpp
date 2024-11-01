//
// Created by Pixeluted on 01/11/2024.
//
#pragma once

#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Scripts final : public RbxStu::StuLuau::Environment::Library {

        static int setidentity(lua_State* L);

        static int getidentity(lua_State* L);


        ~Scripts() override = default;

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
}
