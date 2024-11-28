//
// Created by Dottik on 26/11/2024.
//

#include "Renderable.hpp"

#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

namespace RbxStu::Render {
    float Renderable::GetDeltaTime(ImGuiContext *pContext) const {
        return pContext->IO.DeltaTime;
    }

    void Renderable::PushSeparator() {
        // Stolen from land.
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(ImGui::GetCursorScreenPos().x - 9999, ImGui::GetCursorScreenPos().y),
            ImVec2(ImGui::GetCursorScreenPos().x + 9999, ImGui::GetCursorScreenPos().y),
            ImGui::GetColorU32(ImGuiCol_Border));
        ImGui::Dummy(ImVec2(0.f, 5.f));
    }

    void Renderable::DisableRender() {
        this->m_bIsRenderingEnabled = false;
    }

    void Renderable::EnableRender() {
        this->m_bIsRenderingEnabled = true;
    }

    bool Renderable::IsRenderingEnabled() {
        const auto isEditAvailable = RbxStu::Scheduling::TaskSchedulerOrchestrator::GetSingleton()->
                GetTaskScheduler()->IsDataModelActive(RBX::DataModelType::DataModelType_Edit);

        if (!isEditAvailable && this->m_bIsRenderingEnabled) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Graphics,
                      "Disabling RbxStu::Render::Renderable, no Edit DataModel available, graphics APIs cannot be used.");
            this->DisableRender();
        }

        return this->m_bIsRenderingEnabled && isEditAvailable;
    }

    void Renderable::Render(ImGuiContext *pContext) {
    }

    void Renderable::OnKeyPressed(ImmediateGui::VirtualKey key) {
    }

    void Renderable::OnKeyReleased(ImmediateGui::VirtualKey key) {
    }
}
