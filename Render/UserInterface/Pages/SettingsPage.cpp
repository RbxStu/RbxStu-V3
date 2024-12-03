//
// Created by Dottik on 28/11/2024.
//

#include "SettingsPage.hpp"

#include "FastFlags.hpp"

namespace RbxStu::Render::UI::Pages {

    void SettingsPage::Render(ImGuiContext *pContext) {
        ImGui::Text(">> RbxStu Information");

        if (RbxStu::FastFlags::SFlagRbxCrashKey.IsDefined())
            ImGui::Text("Current RbxStu::RBXCRASH key: '%s' (Default)",
                        RbxStu::FastFlags::SFlagRbxCrashKey.GetValue().c_str());
        else
            ImGui::Text("Current RbxStu::RBXCRASH key: '%s'", RbxStu::FastFlags::SFlagRbxCrashKey.GetValue().c_str());

        this->m_bEnableExperimentalFunctions = FastFlags::FFlagEnableExperimentalLuauFunctions.GetValue();
        ImGui::Checkbox("Enable Experimental Functions", &this->m_bEnableExperimentalFunctions);
        FastFlags::FFlagEnableExperimentalLuauFunctions.SetValue(this->m_bEnableExperimentalFunctions);

        Renderable::PushSeparator();

        ImGui::Text("RbxStu FastFlags Management");

        if (ImGui::Button("Reload FastFlags"))
            FastFlagsManager::GetSingleton()->ReloadFlags();

        if (ImGui::Button("Write FastFlags"))
            FastFlagsManager::GetSingleton()->WriteFlags();

        Renderable::PushSeparator();

        Renderable::Render(pContext);
    }
} // namespace RbxStu::Render::UI::Pages
