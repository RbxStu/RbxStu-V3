//
// Created by Pixeluted on 29/11/2024.
//

#include "Instances.hpp"

#include <Logger.hpp>
#include <Utilities.hpp>
#include <cstring>
#include <format>

namespace RbxStu::StuLuau::Environment::UNC {
    struct WeakThreadRef_Connections {
        std::int64_t *_Refs;
        lua_State *thread;
        std::int64_t thread_ref;
        std::int64_t objectId;
    };

    struct FunctionSlot {
        void *vft;
        char filler[104];
        RBX::Lua::WeakThreadRef *objRef;
    };

    struct ConnectionSlot {
        int Connected;
        int unk0;
        void *unk1;
        ConnectionSlot *pNext;
        int unk2;
        int unk3;
        std::vector<ConnectionSlot> *pConnections;
        void *unk4;
        FunctionSlot *pFunctionSlot;
    };

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
        lua_pushcfunction(L, [](lua_State *) -> int { return 0; }, "");
        lua_call(L, 2, 1);

        const auto rawConnection = reinterpret_cast<ConnectionSlot *>(
                *reinterpret_cast<std::uintptr_t *>(*static_cast<std::uintptr_t *>(lua_touserdata(L, -1)) + 0x10));
        RbxStuLog(Warning, Anonymous, std::format("Root: {}", (void *) rawConnection));

        lua_getfield(L, -1, "Disconnect");
        lua_pushvalue(L, -2);
        lua_call(L, 1, 0);

        ConnectionSlot *slot = rawConnection;
        while (slot != nullptr) {
            RbxStuLog(Warning, Anonymous, std::format("Found Connection: {}", (void *) slot));

            RbxStuLog(Warning, Anonymous, std::format("Refs: {}", (void *) slot->pFunctionSlot->objRef->_Refs));
            RbxStuLog(Warning, Anonymous,
                      std::format("Connected Lua State: {}", (void *) slot->pFunctionSlot->objRef->thread));
            RbxStuLog(Warning, Anonymous,
                      std::format("Thread Reference: {}", (void *) slot->pFunctionSlot->objRef->thread_ref));
            RbxStuLog(Warning, Anonymous,
                      std::format("Function Reference: {}", (void *) slot->pFunctionSlot->objRef->objectId));


            slot = slot->pNext;
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
