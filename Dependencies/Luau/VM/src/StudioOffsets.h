//
// Created by Dottik & Pixeluted on 3/4/2024.
//
#pragma once

#include <cstdint>
#include <memory>
#include <map>
#include <mutex>
#include <shared_mutex>

struct lua_TValue;

class RbxStuOffsets final
{
    static std::shared_ptr<RbxStuOffsets> ptr;
    static std::shared_mutex __rbxstuoffsets__sharedmutex__;

public:
    enum class OffsetKey : uint8_t
    {
        luau_execute,
        pseudo2addr,
        luaE_newthread,
        lua_newthread,
        FromLuaState,
        freeblock,
        luaD_throw,
        luaD_rawrununprotected,
        luaC_step,
        luaV_gettable,
        luaV_settable,
        luaO_nilobject,
        _luaH_dummynode,
        lua_pushvalue,
        luaH_new,
        luau_load,

        RBX_ScriptContext_resume,
        RBX_ScriptContext_getDataModel,
        RBX_ScriptContext_validateThreadAccess,
        RBX_ScriptContext_task_spawn,
        RBX_ScriptContext_task_defer,
        RBX_ScriptContext_getGlobalState,

        RBX_Instance_pushInstance,
        RBX_Instance_getTopAncestor,
        RBX_Instance_removeAllChildren,

        RBX_BasePart_getNetworkOwner,
        RBX_BasePart_fireTouchSignals,

        RBXCRASH
    };

    std::map<OffsetKey, void*> offsets{};


    __declspec(dllexport) static std::shared_ptr<RbxStuOffsets> GetSingleton();

    __declspec(dllexport) void* GetOffset(OffsetKey key);
};

__declspec(dllexport) extern std::string_view OffsetKeyToString(RbxStuOffsets::OffsetKey offsetKey);

__declspec(dllexport) extern void SetOffset(RbxStuOffsets::OffsetKey key, void* func);


struct lua_State;

namespace RBX::Studio::FunctionTypes
{
    using luau_execute = void(__fastcall*)(lua_State* L);
    using pseudo2addr = lua_TValue*(__fastcall*)(lua_State* L, int32_t lua_index);
    using task_defer = int(__fastcall*)(lua_State* L);
    using luaE_newthread = lua_State*(__fastcall*)(lua_State* L);
    using lua_newthread = lua_State*(__fastcall*)(lua_State* L);
    using FromLuaState = void(__fastcall*)(lua_State* LP, lua_State* L);
    using freeBlock = void(__fastcall*)(lua_State* L, int32_t sizeClass, void* block);
    using luaD_throw = void(__fastcall*)(lua_State* L, int32_t errcode);
    using luaD_rawrununprotected = int32_t(__fastcall*)(lua_State* L, void (*PFunc)(lua_State* L, void* ud), void* ud);
    using luaC_step = size_t(__fastcall*)(lua_State* L, bool assist);
    using fireproximityprompt = void(__fastcall*)(void* proximityPrompt);
    using pushinstance = std::uintptr_t(__fastcall*)(lua_State* L, void* instance);
    using luaV_gettable = void(__fastcall*)(lua_State* L, const void* t, const void* key, void* val);
    using luaV_settable = void(__fastcall*)(lua_State* L, const void* t, const void* key, void* val);
}; // namespace RBX::Studio::FunctionTypes


/*
 *  How to get this to compile when updating Luau?
 *      - Modify lobject.cpp and lobject.h to use Studios' luaO_nilobject, same thing with ltable.cpp and ltable.h and luaH_dummynode, as well as
 * modifying lvm.cpp to use luau_execute. You must use luau_load when compiling code, for anyone using this to develop anything.
 */
