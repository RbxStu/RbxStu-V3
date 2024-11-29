//
// Created by Dottik on 29/11/2024.
//

#include "Script.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "Luau/Compiler.h"
#include "TypeDefinitions.hpp"

namespace RbxStu::Roblox {
    std::string Script::GetScriptHash() const {
        return *reinterpret_cast<std::string *>(reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x190);
    }
    std::string Script::GetSource() const {
        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit))
            return {};

        const auto protectedString =
                *reinterpret_cast<std::uintptr_t *>(reinterpret_cast<std::uintptr_t>(this->GetRealStructure()) + 0x1B8);
        return std::string(*reinterpret_cast<char **>(protectedString + 0x10),
                           *reinterpret_cast<std::int64_t *>(protectedString + 0x20));
    }

    std::string Script::GetBytecode() const {
        if (!RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler()->IsDataModelActive(
                    RBX::DataModelType::DataModelType_Edit))
            return {};

        const auto src = this->GetSource();
        auto opts = Luau::CompileOptions{};
        opts.debugLevel = 2;
        opts.optimizationLevel = 1;
        return Luau::compile(src, opts);
    }
} // namespace RbxStu::Roblox
