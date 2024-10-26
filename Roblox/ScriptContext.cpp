//
// Created by Dottik on 14/10/2024.
//

#include "ScriptContext.hpp"

#include <Logger.hpp>
#include <StudioOffsets.h>
#include <Utilities.hpp>
#include <Scanners/Rbx.hpp>

#include "TypeDefinitions.hpp"
#include "StuLuau/ExecutionEngine.hpp"

namespace RbxStu::Roblox {
    std::shared_ptr<RbxStu::Roblox::ScriptContext> ScriptContext::FromWaitingHybridScriptsJob(
        void *waitingHybridScriptsJob) {
        if (!Utilities::IsPointerValid(
                reinterpret_cast<void ***>(reinterpret_cast<std::uintptr_t>(waitingHybridScriptsJob) + 0x1F8)) ||
            !Utilities::IsPointerValid(
                *reinterpret_cast<void ***>(reinterpret_cast<std::uintptr_t>(waitingHybridScriptsJob) + 0x1F8))) {
            return {
                nullptr
            };
        }

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

    void ScriptContext::ResumeThread(RBX::Lua::WeakThreadRef *resumptionContext,
                                     const StuLuau::YieldResult &YieldResult) {
        RbxStuLog(RbxStu::LogType::Information, RbxStu::Roblox_ScriptContext,
                  std::format(
                      "Performing yield using RBX::ScriptContext::resume; Yield Information: nRet: {}; targetL: {}; successful: {}; errorMessage: {}"
                      , YieldResult.dwNumberOfReturns, reinterpret_cast<void*>(resumptionContext->thread), YieldResult.
                      bIsSuccess, (YieldResult.szErrorMessage.has_value() ? YieldResult.szErrorMessage.value() :
                          "no error described")));

        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto resume = reinterpret_cast<r_RBX_ScriptContext_resume>(offsetContainer->
            GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_resume));

        const auto pointerOffset = RbxStu::Scanners::RBX::GetSingleton()->GetRbxPointerOffset(
            RbxStu::Scanners::RBX::PointerOffsets::RBX_ScriptContext_resume);

        if (!pointerOffset.has_value())
            throw std::exception("How did you even get here? This should NOT happen, it is IMPOSSIBLE buddy.");

        std::int64_t status[0x2];

        std::string errorMessage = "No error from RbxStu::StuLuau";
        if (!YieldResult.szErrorMessage.has_value())
            errorMessage = YieldResult.szErrorMessage.value();

        resume(this->GetRbxPointer(), status, &resumptionContext, YieldResult.dwNumberOfReturns, YieldResult.bIsSuccess,
               errorMessage.c_str());
    }
}
