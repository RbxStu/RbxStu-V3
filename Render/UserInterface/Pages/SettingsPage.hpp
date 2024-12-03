//
// Created by Dottik on 28/11/2024.
//

#pragma once
#include "FastFlags.hpp"
#include "Miscellaneous/ListenableEvent.hpp"
#include "Render/Renderable.hpp"

namespace RbxStu::Render::UI::Pages {
    class SettingsPage final : public RbxStu::Render::Renderable {
        static std::shared_ptr<SettingsPage> m_pInstance;
        std::uintptr_t m_connectionId;
        std::string m_szRbxCrashKey;
        bool m_bEnableExperimentalFunctions = false;

        static void OnFastFlagsReloaded(const Miscellaneous::EventArgument<OnFlagsReloaded> &obj);

    public:
        static std::shared_ptr<SettingsPage> GetSingleton();

        SettingsPage();
        ~SettingsPage() override = default;

        void Render(ImGuiContext *pContext) override;
    };
} // namespace RbxStu::Render::UI::Pages
