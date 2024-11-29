//
// Created by Pixeluted on 29/11/2024.
//
#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Instances final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int getconnections(lua_State *L);

        ~Instances() override = default;

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
} // namespace RbxStu::StuLuau::Environment::UNC
