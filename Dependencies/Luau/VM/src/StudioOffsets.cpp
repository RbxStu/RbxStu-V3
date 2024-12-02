//
// Created by Dottik on 3/4/2024.
//

#include "StudioOffsets.h"
#include "memory"
#include "shared_mutex"
#include "../../../../Logger.hpp"

std::shared_ptr<RbxStuOffsets> RbxStuOffsets::ptr;
std::shared_mutex RbxStuOffsets::__rbxstuoffsets__sharedmutex__;

__declspec(dllexport) std::string_view OffsetKeyToString(const RbxStuOffsets::OffsetKey offsetKey)
{
    switch (offsetKey)
    {
    case RbxStuOffsets::OffsetKey::luau_execute:
        return "luau_execute";
    case RbxStuOffsets::OffsetKey::pseudo2addr:
        return "pseudo2addr";
    case RbxStuOffsets::OffsetKey::luaE_newthread:
        return "luaE_newthread";
    case RbxStuOffsets::OffsetKey::lua_newthread:
        return "lua_newthread";
    case RbxStuOffsets::OffsetKey::FromLuaState:
        return "RBX::ScriptContext::userthread_callback";
    case RbxStuOffsets::OffsetKey::freeblock:
        return "freeblock";
    case RbxStuOffsets::OffsetKey::luaD_throw:
        return "luaD_throw";
    case RbxStuOffsets::OffsetKey::luaD_rawrununprotected:
        return "luaD_rawrununprotected";
    case RbxStuOffsets::OffsetKey::luaC_step:
        return "luaC_step";
    case RbxStuOffsets::OffsetKey::luaV_gettable:
        return "luaV_gettable";
    case RbxStuOffsets::OffsetKey::luaV_settable:
        return "luaV_settable";
    case RbxStuOffsets::OffsetKey::luaO_nilobject:
        return "luaO_nilobject";
    case RbxStuOffsets::OffsetKey::_luaH_dummynode:
        return "luaH_dummynode";
    case RbxStuOffsets::OffsetKey::lua_pushvalue:
        return "lua_pushvalue";
    case RbxStuOffsets::OffsetKey::luaH_new:
        return "luaH_new";
    case RbxStuOffsets::OffsetKey::luau_load:
        return "luau_load";
    case RbxStuOffsets::OffsetKey::RBX_ScriptContext_resume:
        return "RBX::ScriptContext::resume";
    case RbxStuOffsets::OffsetKey::RBX_ScriptContext_getDataModel:
        return "RBX::ScriptContext::getDataModel";
    case RbxStuOffsets::OffsetKey::RBX_ScriptContext_validateThreadAccess:
        return "RBX::ScriptContext::validateThreadAccess";
    case RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_spawn:
        return "RBX::ScriptContext::task_spawn";
    case RbxStuOffsets::OffsetKey::RBX_ScriptContext_task_defer:
        return "RBX::ScriptContext::task_defer";
    case RbxStuOffsets::OffsetKey::RBX_ScriptContext_getGlobalState:
        return "RBX::ScriptContext::getGlobalState";
    case RbxStuOffsets::OffsetKey::RBX_Instance_pushInstance:
        return "RBX::Instance::pushInstance";
    case RbxStuOffsets::OffsetKey::RBX_Instance_getTopAncestor:
        return "RBX::Instance::getTopAncestor";
    case RbxStuOffsets::OffsetKey::RBX_Instance_removeAllChildren:
        return "RBX::Instance::removeAllChildren";
    case RbxStuOffsets::OffsetKey::RBX_BasePart_getNetworkOwner:
        return "RBX::BasePart::getNetworkOwner";
    case RbxStuOffsets::OffsetKey::RBX_BasePart_fireTouchSignals:
        return "RBX::BasePart::fireTouchSignals";
    case RbxStuOffsets::OffsetKey::RBX_ProximityPrompt_onTriggered:
        return "RBX::ProximityPrompt::onTriggered";
    case RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireClick:
        return "RBX::ClickDetector::fireClick";
    case RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireRightClick:
        return "RBX::ClickDetector::fireRightClick";
    case RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireMouseHover:
        return "RBX::ClickDetector::fireMouseHover";
    case RbxStuOffsets::OffsetKey::RBX_ClickDetector_fireMouseLeave:
        return "RBX::ClickDetector::fireMouseLeave";
    case RbxStuOffsets::OffsetKey::RBXCRASH:
        return "global::RBXCRASH";
    }

    RbxStuLog(
        RbxStu::Warning,
        RbxStu::StudioOffsets,
        std::format("An offset key {} has no conversion to a string, you may want to fix that!", (uint8_t)(offsetKey))
    );
    return "unknown";
};

__declspec(dllexport) std::shared_ptr<RbxStuOffsets> RbxStuOffsets::GetSingleton()
{
    std::lock_guard lock{__rbxstuoffsets__sharedmutex__};
    if (ptr == nullptr)
        ptr = std::make_shared<RbxStuOffsets>();
    return ptr;
}

__declspec(dllexport) void* RbxStuOffsets::GetOffset(OffsetKey key)
{
    auto it = this->offsets.find(key);
    if (it != this->offsets.end())
        return it->second;

    // printf("[[DEBUG]] invalid key into offset map -> %d (%s); returning nullptr!\n", key, OffsetKeyToString(key).data());
    return nullptr;
}

__declspec(dllexport) void SetOffset(RbxStuOffsets::OffsetKey key, void* func)
{
    RbxStuOffsets::GetSingleton()->offsets[key] = func;
}
