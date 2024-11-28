//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <memory>

#include "Render/Renderable.hpp"
#include "Miscellaneous/Initializable.hpp"
#include "Render/ImmediateGui/PagedWindow.hpp"
#include "Scheduling/Job/ImguiRenderJob.hpp"

namespace RbxStu::Render {
    class UserInterface final : public Miscellaneous::Initializable, public Render::Renderable {
        static std::shared_ptr<UserInterface> pInstance;
        std::shared_ptr<Scheduling::Jobs::ImguiRenderJob> m_renderJob;
        std::shared_ptr<RbxStu::Render::UI::PagedWindow> m_pPagedWindow;

    public:
        UserInterface() {
            this->m_pPagedWindow = nullptr;
            this->m_renderJob = nullptr;
        }

        static std::shared_ptr<UserInterface> GetSingleton();

        bool Initialize() override;

        void Render(ImGuiContext *pContext) override;

        void OnKeyPressed(ImmediateGui::VirtualKey key) override;

        void OnKeyReleased(ImmediateGui::VirtualKey key) override;

        ~UserInterface() override = default;
    };
}
