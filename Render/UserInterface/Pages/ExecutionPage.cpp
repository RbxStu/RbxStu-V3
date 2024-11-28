//
// Created by Dottik on 27/11/2024.
//

#include "ExecutionPage.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "StuLuau/ExecutionEngine.hpp"

#include <misc/cpp/imgui_stdlib.h>

namespace RbxStu::Render::UI::Pages {
    ExecutionPage::ExecutionPage() {
        this->m_executeTextBuffer.reserve(100000);
        this->m_executeTextBuffer = "-- Welcome to RbxStu V3";
        this->m_selectedExecutionSecurity.reserve(16);
        this->m_dwCurrentlySelectedExecutionSecurity = 0;
        this->m_executionSecurities = {
            "LocalScript",
            "RobloxScript",
            "Plugin",
            "RobloxPlugin",
            "RobloxExecutor"
        };
    }

    void ExecutionPage::ExecuteBuffer(RBX::DataModelType dwTargetDataModel) {
        // Function name sounds scary asf, lets keep it to scare anyone who doesn't know what it does :)
        const auto taskScheduler = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto execEngine = taskScheduler->GetExecutionEngine(dwTargetDataModel);

        if (execEngine == nullptr) return;

        StuLuau::ExecutionSecurity dwExecutionSecurity{};

        if (strcmp("LocalScript", this->m_selectedExecutionSecurity.data()) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::LocalScript;
        if (strcmp("RobloxScript", this->m_selectedExecutionSecurity.data()) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::RobloxScript;
        if (strcmp("Plugin", this->m_selectedExecutionSecurity.data()) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::Plugin;
        if (strcmp("RobloxPlugin", this->m_selectedExecutionSecurity.data()) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::RobloxPlugin;
        if (strcmp("RobloxExecutor", this->m_selectedExecutionSecurity.data()) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::RobloxExecutor;

        execEngine->ScheduleExecute(false, this->m_executeTextBuffer, dwExecutionSecurity, true);
    }

    void ExecutionPage::Render(ImGuiContext *pContext) {
        ImGui::InputTextMultiline("Input Luau", &this->m_executeTextBuffer,
                                  ImVec2(400, 200));

        if (ImGui::Button("Execute in Client DataModel"))
            this->ExecuteBuffer(RBX::DataModelType::DataModelType_PlayClient);

        if (ImGui::Button("Execute in Server DataModel"))
            this->ExecuteBuffer(RBX::DataModelType::DataModelType_PlayServer);

        if (ImGui::Button("Execute in Standalone DataModel"))
            this->ExecuteBuffer(RBX::DataModelType::DataModelType_MainMenuStandalone);

        if (ImGui::Button("Execute in Edit DataModel"))
            this->ExecuteBuffer(RBX::DataModelType::DataModelType_Edit);

        ImGui::Combo("Execution Security (Run As)", &this->m_dwCurrentlySelectedExecutionSecurity,
                     this->m_executionSecurities.data(), this->m_executionSecurities.size());

        Renderable::Render(pContext); // call base.
    }
}
