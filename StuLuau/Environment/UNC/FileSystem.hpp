//
// Created by Dottik on 16/11/2024.
//

#pragma once

#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class FileSystem final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int writefile(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
} // RbxStu::StuLuau::Environment::UNC
