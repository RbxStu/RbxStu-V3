//
// Created by Dottik on 28/11/2024.
//

#include "SettingsPage.hpp"

#include "FastFlags.hpp"

namespace RbxStu::Render::UI::Pages {
    std::shared_ptr<SettingsPage> SettingsPage::m_pInstance;


    std::shared_ptr<SettingsPage> SettingsPage::GetSingleton() { return SettingsPage::m_pInstance; }

    void SettingsPage::OnFastFlagsReloaded(const Miscellaneous::EventArgument<OnFlagsReloaded> &obj) {
        const auto pSelf = SettingsPage::GetSingleton();

        pSelf->m_bEnableExperimentalFunctions =
                FastFlags::FFlagEnableExperimentalLuauFunctions.GetValue(obj.value->pManager);
        pSelf->m_szRbxCrashKey = FastFlags::SFlagRbxCrashKey.GetValue(obj.value->pManager);
    }


    SettingsPage::SettingsPage() {
        auto manager = RbxStu::FastFlagsManager::GetSingleton();
        this->m_connectionId = manager->OnFastFlagsReloaded.AttachFunction(SettingsPage::OnFastFlagsReloaded);

        SettingsPage::m_pInstance =
                std::shared_ptr<SettingsPage>(this); // WARNING: DO NOT MOVE OR YOU WILL BE ASSASSINATED, THIS HAS A
                                                     // SIDE-EFFECT, THE ORDER MATTERS :angry:
        SettingsPage::OnFastFlagsReloaded({std::make_unique<OnFlagsReloaded>(manager)}); // Initial fetch value.
    }

    void SettingsPage::Render(ImGuiContext *pContext) {
        ImGui::Text(">> RbxStu Information");

        if (RbxStu::FastFlags::SFlagRbxCrashKey.IsDefined())
            ImGui::Text("Current RbxStu::RBXCRASH key: '%s' (Default)",
                        RbxStu::FastFlags::SFlagRbxCrashKey.GetValue().c_str());
        else
            ImGui::Text("Current RbxStu::RBXCRASH key: '%s'", RbxStu::FastFlags::SFlagRbxCrashKey.GetValue().c_str());

        if (ImGui::Checkbox("Enable Experimental Functions", &this->m_bEnableExperimentalFunctions))
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
