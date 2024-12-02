//
// Created by Dottik on 16/11/2024.
//

#pragma once

#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class FileSystem final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int writefile(lua_State *L);

        static int appendfile(lua_State *L);

        static int readfile(lua_State *L);

        static int makefolder(lua_State *L);

        static int delfolder(lua_State *L);

        static int delfile(lua_State *L);

        static int isfile(lua_State *L);

        static int isfolder(lua_State *L);

        static int listfiles(lua_State *L);

        static int loadfile(lua_State *L);

        static int dofile(lua_State *L);

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
} // namespace RbxStu::StuLuau::Environment::UNC
