//
// Created by Pixeluted on 29/11/2024.
//

#include "Instances.hpp"

#include <Logger.hpp>
#include <cstring>
#include <format>

namespace RbxStu::StuLuau::Environment::UNC {

    int Instances::getconnections(lua_State *L) {
        luaL_checktype(L, 1, LUA_TUSERDATA);

        lua_getglobal(L, "typeof");
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        if (strcmp(lua_tostring(L, -1), "RBXScriptSignal") != 0)
            luaL_argerrorL(L, 1, "Expected RBXScriptSignal");
        lua_pop(L, 1);

        // Connect with a mock function, to get a connection instance

        lua_getfield(L, 1, "Connect");
        lua_pushvalue(L, 1);
        lua_pushcfunction(L, [](lua_State *) -> int {
            return 0;
        }, "");
        lua_call(L, 2, 1);

        const auto rawConnection = *static_cast<uintptr_t*>(lua_touserdata(L, -1));
        auto currentConnection = *reinterpret_cast<uintptr_t*>(rawConnection + 0x10);

        lua_getfield(L, -1, "Disconnect");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);

        while (currentConnection != 0x0) {
            RbxStuLog(Warning, Anonymous, std::format("Found Connection: 0x{:x}", currentConnection));
            currentConnection = *reinterpret_cast<uintptr_t*>(currentConnection + 0x10);
        }

        return 0;
    }

    const char *Instances::GetLibraryName() { return "instances"; }

    bool Instances::PushToGlobals() { return true; }

    const luaL_Reg *Instances::GetFunctionRegistry() {
        static luaL_Reg libreg[] = {{"getconnections", getconnections},

                                    {nullptr, nullptr}};

        return libreg;
    }

} // namespace RbxStu::StuLuau::Environment::UNC
