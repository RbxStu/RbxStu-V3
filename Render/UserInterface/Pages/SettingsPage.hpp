//
// Created by Dottik on 28/11/2024.
//

#pragma once
#include "Render/Renderable.hpp"

namespace RbxStu::Render::UI::Pages {
    class SettingsPage final : public RbxStu::Render::Renderable {
    public:
        ~SettingsPage() override = default;

        void Render(ImGuiContext *pContext) override;
    };
}
