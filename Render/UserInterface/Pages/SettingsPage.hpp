//
// Created by Dottik on 28/11/2024.
//

#pragma once
#include "Render/Renderable.hpp"

namespace RbxStu::Render::UI::Pages {
    class SettingsPage final : public RbxStu::Render::Renderable {
        bool m_bEnableExperimentalFunctions = false;
    public:
        ~SettingsPage() override = default;

        void Render(ImGuiContext *pContext) override;
    };
}
