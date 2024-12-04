//
// Created by Dottik on 29/11/2024.
//

#include "Script.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include <Windows.h>
#include <string>
#include "Luau/Compiler.h"
#include "TypeDefinitions.hpp"

namespace RbxStu::Roblox {
    Script::Script(void *pNative, const RbxStu::Roblox::Script::ScriptKind kind) : OpaqueClass(pNative) {
        this->m_scriptKind = kind;
    }

    std::string Script::GetScriptHash() const {
        return *reinterpret_cast<std::string *>(reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x190);
    }

    std::string ReadProtectedString(void *pProtectedString) {
        return std::string(
                *reinterpret_cast<char **>(reinterpret_cast<std::uintptr_t>(pProtectedString) + 0x10),
                *reinterpret_cast<std::int64_t *>(reinterpret_cast<std::uintptr_t>(pProtectedString) + 0x20));
    };

    std::optional<std::string> Script::GetSource() const {
        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit)) {
            return {};
        }

        // Source access, required in ONLY in non-Team Create environments.

        const auto protectedString =
                *reinterpret_cast<uintptr_t *>(reinterpret_cast<uintptr_t>(this->GetRealStructure()) +
                                               (this->m_scriptKind == ScriptKind::ModuleScript ? 0x168 : 0x1B8));
        if (!protectedString || IsBadReadPtr(reinterpret_cast<void *>(protectedString), 0x10))
            return std::nullopt;

        const auto stringSize = *reinterpret_cast<int64_t *>(protectedString + 0x20);
        const auto stringPointer = *reinterpret_cast<const char **>(protectedString + 0x10);

        auto scriptSource = std::string(stringPointer, stringSize);
        return scriptSource;
    }

    std::optional<std::string> Script::GetBytecode() const {
        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit)) {

            switch (this->m_scriptKind) {
                case ScriptKind::LocalScript: {
                    /*
                     *  Bytecode offset: 0x1C0
                     */
                    const auto protectedString =
                            *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(this->GetRealStructure()) + 0x1C0);
                    if (!protectedString || IsBadReadPtr(protectedString, 0x10))
                        return std::nullopt;

                    return ReadProtectedString(protectedString);
                }
                case ScriptKind::ModuleScript: {
                    /*
                     *  Bytecode offset: 0x168
                     */
                    const auto protectedString =
                            *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(this->GetRealStructure()) + 0x168);
                    if (!protectedString || IsBadReadPtr(protectedString, 0x10))
                        return std::nullopt;

                    return ReadProtectedString(protectedString);
                }
                case ScriptKind::Script:
                default:
                    return std::nullopt;
            }

            return std::nullopt;
        }

        const auto src = this->GetSource();
        if (!src.has_value())
            return std::nullopt;

        // We need to check if it is a roblox script module script, bc if yes, then the source is bytecode
        if (this->m_scriptKind == ScriptKind::ModuleScript &&
            *reinterpret_cast<int32_t *>(reinterpret_cast<uintptr_t>(this->GetRealStructure()) + 0x1B0) == 1) {
            return src.value();
        }

        auto opts = Luau::CompileOptions{};
        opts.debugLevel = 2;
        opts.optimizationLevel = 1;
        return Luau::compile(src.value(), opts);
    }
} // namespace RbxStu::Roblox
