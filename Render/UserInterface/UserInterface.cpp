//
// Created by Dottik on 27/11/2024.
//

#include "UserInterface.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskScheduler.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include <memory>
#include <mutex>

#include "Pages/CreditsPage.hpp"
#include "Pages/ExecutionPage.hpp"
#include "Pages/SettingsPage.hpp"
#include "Render/ImmediateGui/Keycodes.hpp"
#include "Render/ImmediateGui/PagedWindow.hpp"
#include "Render/RenderableStub.hpp"
#include "Scheduling/Job/ImguiRenderJob.hpp"

namespace RbxStu::Render {
    std::shared_ptr<UserInterface> UserInterface::pInstance;

    static std::mutex g_UserInterfaceLock{};

    std::shared_ptr<UserInterface> UserInterface::GetSingleton() {
        if (!UserInterface::pInstance)
            UserInterface::pInstance = std::make_shared<UserInterface>();

        if (!UserInterface::pInstance->IsInitialized()) {
            std::scoped_lock lg{g_UserInterfaceLock};
            if (UserInterface::pInstance->IsInitialized())
                return UserInterface::pInstance;

            UserInterface::pInstance->Initialize();
        }

        return UserInterface::pInstance;
    }

    bool UserInterface::Initialize() {
        if (this->IsInitialized())
            return false;

        const auto orchestrator = Scheduling::TaskSchedulerOrchestrator::GetSingleton();
        const auto taskScheduler = orchestrator->GetTaskScheduler();

        auto imguiRenderJob = taskScheduler->GetSchedulerJob<RbxStu::Scheduling::Jobs::ImguiRenderJob>(
                Scheduling::Jobs::AvailableJobs::ImguiRenderJob);

        if (!imguiRenderJob.has_value())
            return false; // Impossible, but anything can happen in C++ bro

        const auto pages = std::vector<UI::UIPage>{
                UI::UIPage{std::make_shared<RbxStu::Render::UI::Pages::ExecutionPage>(), "Execution"},
                UI::UIPage{std::make_shared<RbxStu::Render::UI::Pages::SettingsPage>(), "Settings"},
                UI::UIPage{std::make_shared<RbxStu::Render::UI::Pages::CreditsPage>(), "Credits"},
        };

        this->m_pPagedWindow = std::make_shared<UI::PagedWindow>(pages, "RbxStu V3", 3);
        this->m_renderJob = imguiRenderJob.value();
        this->m_bIsInitialized = true;
        return true;
    }

    void UserInterface::Render(ImGuiContext *pContext) {
        this->m_pPagedWindow->Render(pContext);

        Renderable::Render(pContext);
    }

    void UserInterface::OnKeyPressed(const ImmediateGui::VirtualKey key) {
        this->m_pPagedWindow->OnKeyPressed(key);

        Renderable::OnKeyPressed(key);
    }

    void UserInterface::OnKeyReleased(const ImmediateGui::VirtualKey key) {
        if (key == RbxStu::Render::ImmediateGui::VirtualKey::INSERT) {
            if (!this->IsRenderingEnabled()) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics, "Enabling Internal UI");
                this->EnableRender();
            } else {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics, "Disabling Internal UI");
                this->DisableRender();
            }
        }

        this->m_pPagedWindow->OnKeyReleased(key);

        Renderable::OnKeyReleased(key);
    }
} // namespace RbxStu::Render
