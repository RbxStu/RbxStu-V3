//
// Created by Dottik on 26/11/2024.
//

#include "Renderable.hpp"

namespace RbxStu::Render {
    std::int64_t Renderable::DeltaTime() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::high_resolution_clock::now() - this->m_lastFrame).count();
    }

    void Renderable::PushSeparator() {
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
        return this->m_bIsRenderingEnabled;
    }

    void Renderable::Render(ImGuiContext *pContext) {
        this->m_lastFrame = std::chrono::high_resolution_clock::now();
    }

    void Renderable::OnKeyPressed(ImmediateGui::VirtualKey key) {
    }

    void Renderable::OnKeyReleased(ImmediateGui::VirtualKey key) {
    }
}
