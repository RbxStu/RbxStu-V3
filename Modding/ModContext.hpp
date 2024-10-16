//
// Created by Dottik on 15/10/2024.
//

#pragma once
#include <Scheduling/TaskScheduler.hpp>

#include "include/ModVersion.hpp"

namespace RbxStu::Modding {
    class __declspec(dllexport, novtable) ModContext final {
        std::string_view m_modName;
        std::string_view m_modIdentifier;
        RbxStu::Modding::ModVersion m_modVersion{};

    public:
        std::shared_ptr<RbxStu::Modding::ModContext> CreateModContext(std::string_view modName, std::string_view modIdentifier,
                   const RbxStu::Modding::ModVersion &version);

        ~ModContext() = default;

        std::shared_ptr<RbxStu::Scheduling::TaskScheduler> GetTaskScheduler();
    };
} // RbxStu::Modding
