//
// Created by Dottik on 27/11/2024.
//

#include "ExecutionPage.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "StuLuau/ExecutionEngine.hpp"

#include <Scheduling/TaskScheduler.hpp>
#include <misc/cpp/imgui_stdlib.h>

namespace RbxStu::Render::UI::Pages {
    ExecutionPage::ExecutionPage() {
        this->m_executeTextBuffer.reserve(100000);
        this->m_executeTextBuffer = "-- Welcome to RbxStu V3";

        this->m_dwCurrentlySelectedExecutionSecurity = 0;
        this->m_executionSecurities = {"LocalScript", "RobloxScript", "Plugin", "RobloxPlugin", "RobloxExecutor"};

        this->m_dwCurrentlySelectedExecutionDataModel = 0;
        this->m_executionDataModels = {"Client", "Server", "Edit", "Standalone"};

        this->isFirstRender = true;
    }

    void ExecutionPage::ExecuteBuffer() {
        StuLuau::ExecutionSecurity dwExecutionSecurity{};
        RBX::DataModelType dwTargetDataModel{};

        const auto szCurrentlySelectedIdentity =
                this->m_executionSecurities[this->m_dwCurrentlySelectedExecutionSecurity];

        const auto szCurrentlySelectedDataModel =
                this->m_executionDataModels[this->m_dwCurrentlySelectedExecutionDataModel];

        if (strcmp("LocalScript", szCurrentlySelectedIdentity) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::LocalScript;
        if (strcmp("RobloxScript", szCurrentlySelectedIdentity) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::RobloxScript;
        if (strcmp("Plugin", szCurrentlySelectedIdentity) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::Plugin;
        if (strcmp("RobloxPlugin", szCurrentlySelectedIdentity) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::RobloxPlugin;
        if (strcmp("RobloxExecutor", szCurrentlySelectedIdentity) == 0)
            dwExecutionSecurity = StuLuau::ExecutionSecurity::RobloxExecutor;

        if (strcmp("Client", szCurrentlySelectedDataModel) == 0)
            dwTargetDataModel = RBX::DataModelType::DataModelType_PlayClient;
        if (strcmp("Server", szCurrentlySelectedDataModel) == 0)
            dwTargetDataModel = RBX::DataModelType::DataModelType_PlayServer;
        if (strcmp("Edit", szCurrentlySelectedDataModel) == 0)
            dwTargetDataModel = RBX::DataModelType::DataModelType_Edit;
        if (strcmp("Standalone", szCurrentlySelectedDataModel) == 0)
            dwTargetDataModel = RBX::DataModelType::DataModelType_MainMenuStandalone;


        // Function name sounds scary asf, lets keep it to scare anyone who doesn't know what it does :)
        const auto taskScheduler = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->GetTaskScheduler();
        const auto execEngine = taskScheduler->GetExecutionEngine(dwTargetDataModel);

        if (execEngine == nullptr) {
            const auto printFunc = reinterpret_cast<r_RBX_Console_StandardOut>(
                    RbxStuOffsets::GetSingleton()->GetOffset(RbxStuOffsets::OffsetKey::RBX_Console_StandardOut));
            const auto formattedMessage = std::format("Cannot execute on {} DataModel! (DataModel isn't available)",
                                                      szCurrentlySelectedDataModel)
                                                  .c_str();

            printFunc(RBX::Console::Warning, formattedMessage);
            return;
        };

        execEngine->ScheduleExecute(false, this->m_executeTextBuffer, dwExecutionSecurity, true);
    }

    void ExecutionPage::Render(ImGuiContext *pContext) {
        ImGui::Text("Execute (Input Luau Code)");

        Renderable::PushSeparator();

        ImGui::InputTextMultiline("", &this->m_executeTextBuffer,
                                  ImVec2(isFirstRender ? 400 : pContext->CurrentWindow->Size.x - 17, 200));

        if (ImGui::Button("Execute Payload"))
            this->ExecuteBuffer();

        ImGui::Combo("Run On", &this->m_dwCurrentlySelectedExecutionDataModel, this->m_executionDataModels.data(),
                     this->m_executionDataModels.size());

        ImGui::Combo("Execution Security", &this->m_dwCurrentlySelectedExecutionSecurity,
                     this->m_executionSecurities.data(), this->m_executionSecurities.size());

        if (this->isFirstRender)
            this->isFirstRender = false;

        Renderable::Render(pContext); // call base.
    }
} // namespace RbxStu::Render::UI::Pages
