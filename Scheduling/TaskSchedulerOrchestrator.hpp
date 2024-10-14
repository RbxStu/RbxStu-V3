//
// Created by Dottik on 13/10/2024.
//

#pragma once
#include <atomic>
#include <memory>

#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Scheduling {
    class TaskSchedulerOrchestrator final {
        static std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator> pInstance;

        std::map<std::string_view, RBX::DataModelJobVFTable **> jobVirtualFunctionTableMap;
        std::atomic_bool m_bIsInitialized;

        using r_RBX_DataModelJob_Step = bool(__fastcall *)(void *self);

        void Initialize();

    public:
        bool IsInitialized();

        static std::shared_ptr<RbxStu::Scheduling::TaskSchedulerOrchestrator> GetSingleton();
    };
} // RbxStu::Scheduling
