//
// Created by Dottik on 29/11/2024.
//

#include "Script.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Luau/Compiler.h"
#include "TypeDefinitions.hpp"

#include <string>

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

    std::string Script::GetSource() const {
        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit)) {
            return {};
        }

        // Source access, required in ONLY in non-Team Create environments.

        const auto protectedString =
                *reinterpret_cast<std::uintptr_t *>(reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x1B8);
        return std::string(*reinterpret_cast<char **>(protectedString + 0x10),
                           *reinterpret_cast<std::int64_t *>(protectedString + 0x20));
    }

    std::string Script::GetBytecode() const {
        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit)) {

            switch (this->m_scriptKind) {
                case ScriptKind::LocalScript: {
                    /*
                     *  Bytecode offset: 0x1C0
                     */
                    const auto protectedString = *reinterpret_cast<void **>(
                            reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x1C0);

                    return ReadProtectedString(protectedString);
                }
                case ScriptKind::ModuleScript: {
                    /*
                     *  Bytecode offset: 0x168
                     */
                    const auto protectedString = *reinterpret_cast<void **>(
                            reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x168);
                    return ReadProtectedString(protectedString);
                }
                case ScriptKind::Script:
                default:
                    return {};
            }

            return {};
        }

        const auto src = this->GetSource();
        auto opts = Luau::CompileOptions{};
        opts.debugLevel = 2;
        opts.optimizationLevel = 1;
        return Luau::compile(src, opts);
    }
} // namespace RbxStu::Roblox
