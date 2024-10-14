//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string_view>

namespace RbxStu::Scanners {
    class RBX {
    public:
        enum class RobloxFunction {
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

        static std::string_view RobloxFunctionToString(RobloxFunction functionId) {
            switch (functionId) {
                case RobloxFunction::RBX_ScriptContext_resume:
                    return "RBX::ScriptContext::resume";
                case RobloxFunction::RBX_ScriptContext_getDataModel:
                    return "RBX::ScriptContext::getDataModel";
                case RobloxFunction::RBX_ScriptContext_validateThreadAccess:
                    return "RBX::ScriptContext::validateThreadAccess";
                case RobloxFunction::RBX_ScriptContext_task_spawn:
                    return "RBX::ScriptContext::task_spawn";
                case RobloxFunction::RBX_ScriptContext_task_defer:
                    return "RBX::ScriptContext::task_defer";
                case RobloxFunction::RBX_ScriptContext_getGlobalState:
                    return "RBX::ScriptContext::getGlobalState";
                case RobloxFunction::RBX_Instance_pushInstance:
                    return "RBX::Instance::pushInstance";
                case RobloxFunction::RBX_Instance_getTopAncestor:
                    return "RBX::Instance::getTopAncestor";
                case RobloxFunction::RBX_Instance_removeAllChildren:
                    return "RBX::Instance::removeAllChildren";
                case RobloxFunction::RBX_BasePart_getNetworkOwner:
                    return "RBX::BasePart::getNetworkOwner";
                case RobloxFunction::RBX_BasePart_fireTouchSignals:
                    return "RBX::BasePart::fireTouchSignals";
                case RobloxFunction::RBXCRASH:
                    return "global::RBXCRASH";
            }
            return "unknown";
        }

    private:
        static std::shared_ptr<RbxStu::Scanners::RBX> pInstance;
        std::atomic_bool m_bIsInitialized;

        std::map<std::string_view, void *> m_FastFlagMap;
        std::map<RbxStu::Scanners::RBX::RobloxFunction, const void *> m_FunctionMap;

        void Initialize();

    public:
        static std::shared_ptr<RbxStu::Scanners::RBX> GetSingleton();

        bool IsInitialized();

        template<typename T>
        T GetFastFlag(const std::string_view fastFlagName) {
            if (!this->m_FastFlagMap.contains(fastFlagName)) {
                return {};
            }

            return reinterpret_cast<T>(this->m_FastFlagMap.at(fastFlagName));
        }

        template<typename T>
        std::optional<T> GetRobloxFunction(const RbxStu::Scanners::RBX::RobloxFunction functionIdentifier) const {
            if (!this->m_FunctionMap.contains(functionIdentifier)) {
                return {};
            }

            return this->m_FunctionMap.at(functionIdentifier);
        }
    };
} // RbxStu::Scanners
