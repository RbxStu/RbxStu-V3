//
// Created by Dottik on 14/10/2024.
//

#include "ScriptContext.hpp"
#include <StudioOffsets.h>
#include <Scanners/Rbx.hpp>

#include "TypeDefinitions.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Roblox {
    std::shared_ptr<RbxStu::Roblox::ScriptContext> ScriptContext::FromWaitingHybridScriptsJob(
        void *waitingHybridScriptsJob) {
        return std::make_shared<RbxStu::Roblox::ScriptContext>(
            *reinterpret_cast<void **>(reinterpret_cast<std::uintptr_t>(waitingHybridScriptsJob) + 0x1F8));
    }

    ScriptContext::ScriptContext(void *scriptContext) {
        this->m_pScriptContext = scriptContext;
    }

    void *ScriptContext::GetRbxPointer() const {
        return this->m_pScriptContext;
    }

    lua_State *ScriptContext::GetGlobalState() {
        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto getGlobalState = reinterpret_cast<r_RBX_ScriptContext_getGlobalState>(offsetContainer->
            GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_getGlobalState));

        const auto scanner = RbxStu::Scanners::RBX::GetSingleton();

        constexpr auto identity = 8ull;
        constexpr auto script = 0ull;
        return getGlobalState(this->m_pScriptContext,
                              &identity, &script);
    }

    void ScriptContext::ResumeThread(RBX::Lua::WeakThreadRef *resumptionContext, const StuLuau::YieldResult &YieldResult) {

    }
}
