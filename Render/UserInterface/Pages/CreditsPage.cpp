//
// Created by Dottik on 29/11/2024.
//

#include "CreditsPage.hpp"

namespace RbxStu::Render::UI::Pages {

    void Collaborator::Render(ImGuiContext *pContext) {
        if (this->m_szRelationWithRbxStu.has_value()) {
            ImGui::Text("%s: @%s (%s)", this->m_szRelationWithRbxStu.value().c_str(), this->m_szDiscordName.c_str(),
                        this->m_szGithubName.c_str());
        } else {
            ImGui::Text("@%s (%s)", this->m_szDiscordName.c_str(), this->m_szGithubName.c_str());
        }

        if (!this->m_contributions.empty())
            for (const auto &contribution: this->m_contributions)
                ImGui::Text(std::format(" ~ {}", contribution).c_str());

        Renderable::Render(pContext);
    }

    CreditsPage::CreditsPage() {
        this->m_contributorList = {
                Collaborator::CreateCollaborator(std::nullopt, "_land_", "landervander", {"ImGui Assistance"}),
                Collaborator::CreateCollaborator(std::nullopt, "makesuredudedies", "MakeSureDudeDies",
                                                 {"Rendering Hooks Assistance"}),
        };

        this->m_dottik = Collaborator::CreateCollaborator("Owner", "usrdottik", "SecondNewtonLaw", {});
        this->m_pixeluted = Collaborator::CreateCollaborator("Co-Owner", "pixeluted", "Pixeluted", {});
    }


    void CreditsPage::Render(ImGuiContext *pContext) {
        ImGui::Text(" ~~ RbxStu V3 Development Team ~~ ");

        this->m_dottik->Render(pContext);
        this->m_pixeluted->Render(pContext);

        ImGui::Text(" ~~ RbxStu V3 Development Team ~~ ");
        Renderable::PushSeparator();

        ImGui::Text(">> Contributors");

        for (const auto &contributor: this->m_contributorList)
            contributor->Render(pContext);


        Renderable::Render(pContext);
    }
} // namespace RbxStu::Render::UI::Pages
