//
// Created by Dottik on 15/10/2024.
//

#include "ModContext.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

namespace RbxStu::Modding {
    std::shared_ptr<RbxStu::Modding::ModContext> ModContext::CreateModContext(const std::string_view modName,
                                                                              const std::string_view modIdentifier,
                                                                              const RbxStu::Modding::ModVersion &
                                                                              version) {
        auto modContext = std::make_shared<RbxStu::Modding::ModContext>();
        modContext->m_modName = modName;
        modContext->m_modIdentifier = modIdentifier;
        modContext->m_modVersion = version;
        return modContext;
    }


    std::shared_ptr<RbxStu::Scheduling::TaskScheduler> ModContext::GetTaskScheduler() {
        return RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
    }
}
