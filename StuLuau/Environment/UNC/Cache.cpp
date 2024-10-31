//
// Created by Dottik on 31/10/2024.
//

#include "Cache.hpp"

#include <Logger.hpp>
#include <Utilities.hpp>

#include "Globals.hpp"

namespace RbxStu::StuLuau::Environment::UNC {
    int Cache::invalidate(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");

        const auto rawUserdata = *static_cast<void **>(lua_touserdata(L, 1));
        const auto rbxPushInstance = RbxStuOffsets::GetSingleton()->GetOffset(
            RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance);

        if (rbxPushInstance == nullptr) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous,
                      "Cannot perform cache.invalidate: Failed to find RBX::Instance::pushInstance.");
            return 0;
        }

        lua_pushlightuserdata(L, rbxPushInstance);
        lua_gettable(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, reinterpret_cast<void *>(rawUserdata));
        lua_pushnil(L);
        lua_settable(L, -3);

        return 0;
    }

    int Cache::replace(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");

        const auto rawUserdata = *static_cast<void **>(lua_touserdata(L, 1));
        const auto rbxPushInstance = RbxStuOffsets::GetSingleton()->GetOffset(
            RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance);

        if (rbxPushInstance == nullptr) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous,
                      "Cannot perform cache.replace: Failed to find RBX::Instance::pushInstance.");
            return 0;
        }

        lua_pushlightuserdata(L, rbxPushInstance);
        lua_gettable(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, rawUserdata);
        lua_pushvalue(L, 2);
        lua_settable(L, -3);

        return 0;
    }

    int Cache::iscached(lua_State *L) {
        Utilities::checkInstance(L, 1, "ANY");

        const auto rawUserdata = *static_cast<void **>(lua_touserdata(L, 1));
        const auto rbxPushInstance = RbxStuOffsets::GetSingleton()->GetOffset(
            RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance);

        if (rbxPushInstance == nullptr) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Anonymous,
                      "Cannot perform cache.iscached: Failed to find RBX::Instance::pushInstance.");
            lua_pushboolean(L, 0);
            return 1;
        }


        lua_pushlightuserdata(L, rbxPushInstance);
        lua_gettable(L, LUA_REGISTRYINDEX);

        lua_pushlightuserdata(L, rawUserdata);
        lua_gettable(L, -2);

        lua_pushboolean(L, lua_type(L, -1) != LUA_TNIL);
        return 1;
    }

    const luaL_Reg *Cache::GetFunctionRegistry() {
        static luaL_Reg functions[] = {
            // Defined in global to prevent polluting global with 'invalidate', 'replace' and 'iscached'
            {"cloneref", RbxStu::StuLuau::Environment::UNC::Globals::cloneref},
            {"compareinstances", RbxStu::StuLuau::Environment::UNC::Globals::compareinstances},

            {"invalidate", RbxStu::StuLuau::Environment::UNC::Cache::invalidate},
            {"replace", RbxStu::StuLuau::Environment::UNC::Cache::replace},
            {"iscached", RbxStu::StuLuau::Environment::UNC::Cache::iscached},
            {nullptr, nullptr}
        };

        return functions;
    }

    bool Cache::PushToGlobals() {
        return false;
    }

    const char *Cache::GetLibraryName() { return "cache"; }
}
