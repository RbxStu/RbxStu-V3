//
// Created by Dottik on 27/11/2024.
//

#include "PagedWindow.hpp"
#include <Logger.hpp>

namespace RbxStu::Render::UI {
    PagedWindow::PagedWindow(const std::vector<UIPage> &pages, const std::string &szWindowName) {
        this->m_pages = pages;
        this->m_currentPageIndex = 0;
        this->m_szWindowName = szWindowName;
    }

    PagedWindow::~PagedWindow() {
        this->m_pages.clear();
    }

    const UIPage &PagedWindow::GetCurrentPage() const {
        return this->m_pages.at(this->m_currentPageIndex);
    }

    void PagedWindow::SetCurrentPage(const int newCurrentPage) {
        if (m_pages.size() > newCurrentPage) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Graphics,
                      "PagedWindow::SetCurrentPage(): Attempted to set the current page into index outside of the available page set, request dropped");
            return;
        }

        this->m_currentPageIndex = newCurrentPage;
    }

    void PagedWindow::RenderPageButtons() {
        for (int i = 0; i < m_pages.size(); i++) {
            const auto &page = m_pages[i];

            if (this->m_currentPageIndex == i) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::Button(page.szPageName.c_str(), ImVec2(75, 25));
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.19f, 0.19f, 0.19f, 0.54f));
                if (ImGui::Button(page.szPageName.c_str(), ImVec2(75, 25)))
                    this->m_currentPageIndex = i;

                ImGui::PopStyleColor();
            }
        }

        ImGui::NextColumn();
    }

    void PagedWindow::Render(ImGuiContext *pContext) {
        ImGui::Begin("RbxStu::Render::UI::PagedWindow", nullptr, ImGuiWindowFlags_NoTitleBar);

        ImGui::Text(this->m_szWindowName.c_str());

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(ImGui::GetCursorScreenPos().x - 9999, ImGui::GetCursorScreenPos().y),
            ImVec2(ImGui::GetCursorScreenPos().x + 9999, ImGui::GetCursorScreenPos().y),
            ImGui::GetColorU32(ImGuiCol_Border));
        ImGui::Dummy(ImVec2(0.f, 5.f));

        this->RenderPageButtons();

        this->m_pages.at(this->m_currentPageIndex).pageRenderer->Render(pContext);

        ImGui::End();

        Renderable::Render(pContext);
    }
}
