//
// Created by Dottik on 25/10/2024.
//

#pragma once
#include "StuLuau/Environment/Library.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    class Globals final : public RbxStu::StuLuau::Environment::Library {
    public:
        static int getgenv(lua_State *L);

        static int getrenv(lua_State *L);

        static int gettenv(lua_State *L);

        static int httpget(lua_State *L);

        static int checkcaller(lua_State *L);

        static int checkcallstack(lua_State *L);

        static int getreg(lua_State *L);

        static int identifyexecutor(lua_State *L);

        static int lz4compress(lua_State *L);

        static int lz4decompress(lua_State *L);

        static int isscriptable(lua_State *L);

        static int setscriptable(lua_State *L);

        static int gethiddenproperty(lua_State *L);

        static int sethiddenproperty(lua_State *L);

        static int getfpscap(lua_State *L);

        static int setfpscap(lua_State *L);

        static int isluau(lua_State *L);

        static int getrawmetatable(lua_State *L);

        static int setrawmetatable(lua_State *L);

        static int getnamecallmethod(lua_State *L);

        static int setnamecallmethod(lua_State *L);

        static int setreadonly(lua_State *L);

        static int isreadonly(lua_State *L);

        static int cloneref(lua_State *L);

        static int compareinstances(lua_State *L);

        static int getcallingscript(lua_State *L);

        static int gethui(lua_State *L);

        static int isnetworkowner(lua_State *L);

        static int firetouchinterest(lua_State *L);

        static int fireproximityprompt(lua_State *L);


        ~Globals() override = default;

        const luaL_Reg *GetFunctionRegistry() override;

        bool PushToGlobals() override;

        const char *GetLibraryName() override;
    };
} // RbxStu::StuLuau::Environment::UNC
