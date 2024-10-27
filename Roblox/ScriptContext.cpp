//
// Created by Dottik on 14/10/2024.
//

#include "ScriptContext.hpp"

#include <Logger.hpp>
#include <StudioOffsets.h>
#include <Utilities.hpp>
#include <Scanners/Rbx.hpp>

#include "DataModel.hpp"
#include "TypeDefinitions.hpp"
#include "StuLuau/ExecutionEngine.hpp"
#include "StuLuau/LuauSecurity.hpp"

namespace RbxStu::Roblox {
    std::shared_ptr<RbxStu::Roblox::ScriptContext> ScriptContext::FromWaitingHybridScriptsJob(
        void *waitingHybridScriptsJob) {
        ;
        if (!Utilities::IsPointerValid(
                reinterpret_cast<void ***>(reinterpret_cast<std::uintptr_t>(waitingHybridScriptsJob) + 0x1F8)) ||
            !Utilities::IsPointerValid(
                *reinterpret_cast<void ***>(reinterpret_cast<std::uintptr_t>(waitingHybridScriptsJob) + 0x1F8))) {
            return {
                nullptr
            };
        }

        return std::make_shared<RbxStu::Roblox::ScriptContext>(
            *reinterpret_cast<void **>(reinterpret_cast<std::uintptr_t>(waitingHybridScriptsJob) + 0x1F8),
            waitingHybridScriptsJob);
    }

    ScriptContext::ScriptContext(void *scriptContext) {
        this->m_pScriptContext = scriptContext;
    }

    ScriptContext::ScriptContext(void *scriptContext, void *backingJob) {
        this->m_pScriptContext = scriptContext;
        this->m_pBackingJob = backingJob;
    }

    void *ScriptContext::GetRbxPointer() const {
        return this->m_pScriptContext;
    }

    lua_State *ScriptContext::GetGlobalState() {
        const auto offsetContainer = RbxStuOffsets::GetSingleton();

        const auto getGlobalState = reinterpret_cast<r_RBX_ScriptContext_getGlobalState>(offsetContainer->
            GetOffset(RbxStuOffsets::OffsetKey::RBX_ScriptContext_getGlobalState));

        const auto scanner = RbxStu::Scanners::RBX::GetSingleton();

        constexpr auto identity = 0ull;
        constexpr auto script = 0ull;
        return getGlobalState(this->m_pScriptContext,
                              &identity, &script);
    }

    void ScriptContext::ResumeThread(RBX::Lua::WeakThreadRef *resumptionContext,
                                     const StuLuau::YieldResult &YieldResult) const {
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

        if (!pointerOffset.has_value()) {
            RbxStuLog(RbxStu::LogType::Error, RbxStu::Roblox_ScriptContext,
                      "FAILED TO FIND POINTER OFFSET FOR RESUME???????????????????????");
            throw std::exception("How did you even get here? This should NOT happen, it is IMPOSSIBLE buddy.");
        }

        std::int64_t status[0x2]{0, 0};

        std::string errorMessage = {};

        if (YieldResult.szErrorMessage.has_value())
            errorMessage = YieldResult.szErrorMessage.value();

        if (!YieldResult.bIsSuccess && errorMessage.empty())
            errorMessage = std::string("No error from RbxStu::StuLuau");

        if (!YieldResult.bIsSuccess)
            lua_pushlstring(resumptionContext->thread, errorMessage.c_str(), errorMessage.size());
        // ROBLOX when resuming will check the stack top for the error message, we don't know why the hell they keep an argument for an error message if they won't use it, lol.

        RBX::PointerOffsetEncryption<void> decryptor{
            this->GetRbxPointer(),
            // StuLuau::LuauSecurity::GetThreadExtraspace(resumptionContext->thread)->sharedExtraSpace->scriptContext
            pointerOffset.value().offset
        };

        const auto lockViolationCrash = ::RbxStu::Scanners::RBX::GetSingleton()->GetFastFlag<bool *>(
            "LockViolationScriptCrash");
        const auto old = *lockViolationCrash;
        *lockViolationCrash = false;
        // Disable thread access checks for this time, the operation we are doing is safe, but ROBLOX does not trust us.

        resume(decryptor.DecodePointerWithOffsetEncryption(pointerOffset.value().encryption), status,
               &resumptionContext, YieldResult.dwNumberOfReturns, !YieldResult.bIsSuccess,
               errorMessage.empty() ? nullptr : errorMessage.c_str());

        *lockViolationCrash = old;

        RbxStuLog(RbxStu::LogType::Information, RbxStu::Roblox_ScriptContext,
                  std::format(
                      "RBX::ScriptContext::resume -> status[0x0] = {}; status[0x1] = {};"
                      , status[0], status[1]));

        if (status[0] == 0) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::Roblox_ScriptContext, "Yield resumption successful.");
        } else if (status[0] == 2) {
            RbxStuLog(RbxStu::LogType::Information, RbxStu::Roblox_ScriptContext, "coroutine is dead?");
        }
    }
}
