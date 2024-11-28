//
// Created by Dottik on 27/11/2024.
//

#include "UserInterface.hpp"

#include <Logger.hpp>
#include <memory>
#include <mutex>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>
#include "Scheduling/Job/ImguiRenderJob.hpp"
#include "Render/ImmediateGui/Keycodes.hpp"

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
        if (this->IsInitialized()) return false;

        const auto orchestrator = Scheduling::TaskSchedulerOrchestrator::GetSingleton();
        const auto taskScheduler = orchestrator->GetTaskScheduler();

        auto imguiRenderJob = taskScheduler->GetSchedulerJob<RbxStu::Scheduling::Jobs::ImguiRenderJob>(
            Scheduling::Jobs::AvailableJobs::ImguiRenderJob);

        if (!imguiRenderJob.has_value()) return false; // Impossible, but anything can happen in C++ bro

        this->m_renderJob = imguiRenderJob.value();
        this->m_bIsInitialized = true;
        return true;
    }

    void UserInterface::Render(ImGuiContext *pContext) {
        ImGui::Begin("Hello, i hate imgui");
        ImGui::Text("Hello, world!");
        ImGui::End();
        Renderable::Render(pContext);
    }

    void UserInterface::OnKeyPressed(const ImmediateGui::VirtualKey key) {
        Renderable::OnKeyPressed(key);
    }

    void UserInterface::OnKeyReleased(const ImmediateGui::VirtualKey key) {
        if (key == RbxStu::Render::ImmediateGui::VirtualKey::INSERT) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics, "Enabling Internal UI");
            if (!this->IsRenderingEnabled())
                this->EnableRender();
            else
                this->DisableRender();
        }
    }
}
