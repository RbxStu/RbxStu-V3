//
// Created by Dottik on 28/11/2024.
//

#include "SettingsPage.hpp"

#include "FastFlags.hpp"

namespace RbxStu::Render::UI::Pages {

    void SettingsPage::Render(ImGuiContext *pContext) {
        ImGui::Text(">> RbxStu Information");

        if (RbxStu::FastFlags::SFlagRbxCrashKey.IsDefined())
            ImGui::Text("Current RbxStu::RBXCRASH key: '%s' %s", RbxStu::FastFlags::SFlagRbxCrashKey.GetValue().c_str(),
                        "(Default)");
        else
            ImGui::Text("Current RbxStu::RBXCRASH key: '%s'", RbxStu::FastFlags::SFlagRbxCrashKey.GetValue().c_str());

        Renderable::PushSeparator();

        Renderable::Render(pContext);
    }
} // namespace RbxStu::Render::UI::Pages
