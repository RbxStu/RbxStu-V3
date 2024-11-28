//
// Created by Dottik on 26/11/2024.
//

#include "Renderable.hpp"

namespace RbxStu::Render {
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
    }

    void Renderable::OnKeyPressed(ImmediateGui::VirtualKey key) {
    }

    void Renderable::OnKeyReleased(ImmediateGui::VirtualKey key) {
    }
}
