//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <array>
#include <Logger.hpp>
#include <memory>

#include "Render/Renderable.hpp"

namespace RbxStu::Render {
    class Renderable;
}

namespace RbxStu::Render::UI {
    struct UIPage {
        std::shared_ptr<RbxStu::Render::Renderable> pageRenderer;
        std::string szPageName;
    };

    class PagedWindow : public RbxStu::Render::Renderable {
        std::string m_szWindowName;
        int m_currentPageIndex;
        std::vector<UIPage> m_pages;

    public:
        explicit PagedWindow(const std::vector<UIPage> &pages,
                             std::string szWindowName) {
            this->m_pages = pages;
            this->m_currentPageIndex = 0;
            this->m_szWindowName = szWindowName;
        }

        ~PagedWindow() {
            this->m_pages.clear();
        }

        const UIPage &GetCurrentPage() {
            return this->m_pages[this->m_currentPageIndex];
        }

        void SetCurrentPage(const int newCurrentPage) {
            if (m_pages.size() > newCurrentPage) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Graphics,
                          "PagedWindow::SetCurrentPage(): Attempted to set the current page into index outside of the available page set, request dropped");
                return;
            }

            this->m_currentPageIndex = newCurrentPage;
        }

        void RenderPageButtons() {
            for (int i = 0; i < m_pages.size(); i++) {
                const auto &page = m_pages[i];

                if (this->m_currentPageIndex == i) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::Button(page.szPageName.c_str(), ImVec2(90, 30));
                    ImGui::PopStyleColor();
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.19f, 0.19f, 0.19f, 0.54f));
                    if (ImGui::Button(page.szPageName.c_str(), ImVec2(90, 30)))
                        this->m_currentPageIndex = i;

                    ImGui::PopStyleColor();
                }
            }
        }

        void Render(ImGuiContext *pContext) override {
            ImGui::Begin("RbxStu::Render::UI::PagedWindow", nullptr, ImGuiWindowFlags_NoTitleBar);

            ImGui::Text(m_szWindowName.c_str());

            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(ImGui::GetCursorScreenPos().x - 9999, ImGui::GetCursorScreenPos().y),
                ImVec2(ImGui::GetCursorScreenPos().x + 9999, ImGui::GetCursorScreenPos().y),
                ImGui::GetColorU32(ImGuiCol_Border));
            ImGui::Dummy(ImVec2(0.f, 5.f));

            this->RenderPageButtons();

            this->m_pages.at(this->m_currentPageIndex).pageRenderer->Render(pContext);

            ImGui::End();

            Renderable::Render(pContext);
        };
    };
}
