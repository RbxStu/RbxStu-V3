//
// Created by Dottik on 27/11/2024.
//

#include "PagedWindow.hpp"
#include <Logger.hpp>

#include "Render/Color4.hpp"
#include "Render/RenderableStub.hpp"

namespace RbxStu::Render::UI {
    PagedWindow::PagedWindow(const std::vector<UIPage> &pages, const std::string &szWindowName,
                             const int rowsPerColumn) {
        this->m_pages = pages;
        this->m_dwCurrentPageIndex = 0;
        this->m_szWindowName = szWindowName;
        this->m_bRenderPageList = true;
        this->m_dwRowsPerColumn = rowsPerColumn;

        if (this->m_pages.size() % rowsPerColumn != 0) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Graphics,
                      "Unsuitale number of rows per column for the given number of pages; Filling with stub pages!");

            const auto realOldSize = this->m_pages.size();
            auto oldSize = this->m_pages.size();
            auto newSize = oldSize;

            while (newSize++ % rowsPerColumn != 0) {
            }

            this->m_pages.reserve(newSize + 1);

            while (oldSize++ < this->m_pages.capacity())
                this->m_pages.emplace_back(std::make_shared<RbxStu::Render::RenderableStub>(), "~~ ~~", true);

            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Graphics,
                      std::format("Adjusted pages to render objects for proper display; Previous this->m_pages.size(): "
                                  "{:d}; Current this->m_pages.size(): {:d}",
                                  realOldSize, newSize));
        }

        this->m_pages.emplace_back(std::make_shared<RbxStu::Render::RenderableStub>(), "~~ ~~", true);
        // this->m_pages.emplace_back(std::make_shared<RbxStu::Render::RenderableStub>(), "~~ ~~"); // Push stub
    }

    PagedWindow::~PagedWindow() { this->m_pages.clear(); }

    const UIPage &PagedWindow::GetCurrentPage() const { return this->m_pages.at(this->m_dwCurrentPageIndex); }

    void PagedWindow::SetCurrentPage(const int newCurrentPage) {
        if (this->m_pages.size() < newCurrentPage) {
            RbxStuLog(
                    RbxStu::LogType::Warning, RbxStu::Graphics,
                    std::format(
                            "PagedWindow::SetCurrentPage(): Attempted to set the current page into index outside of "
                            "the available page set, request dropped; Attempted New Page: {:d}; Pages List Size: {:d}",
                            newCurrentPage, this->m_pages.size()));
            return;
        }

        this->m_dwCurrentPageIndex = newCurrentPage;
    }

    void PagedWindow::RenderPageButtons() {
        // Stolen from land.
        ImGui::Text("%s", this->m_szWindowName.c_str());
        ImGui::Checkbox("Display Page List", &this->m_bRenderPageList);

        if (this->m_bRenderPageList && ImGui::BeginTable("_", this->m_pages.size() / this->m_dwRowsPerColumn,
                                                         ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            for (int row = 0; row < this->m_dwRowsPerColumn; row++) {
                ImGui::TableNextRow();

                for (int column = 0; column < (this->m_pages.size() / this->m_dwRowsPerColumn); column++) {
                    ImGui::TableSetColumnIndex(column);

                    const auto pageIndex = column * this->m_dwRowsPerColumn + row;

                    const auto page = this->m_pages.at(pageIndex);

                    if (page.bIsStub) {
                        ImGui::Dummy(ImVec2(0, 0));
                        continue;
                    }

                    if (this->m_dwCurrentPageIndex == pageIndex) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

                        ImGui::Button(page.szPageName.c_str());

                        ImGui::PopStyleColor();
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.19f, 0.19f, 0.19f, 0.54f));

                        if (ImGui::Button(page.szPageName.c_str()))
                            this->m_dwCurrentPageIndex = pageIndex;

                        ImGui::PopStyleColor();
                    }
                }
            }

            // End the table
            ImGui::EndTable();
        }

        Renderable::PushSeparator();
    }

    void PagedWindow::Render(ImGuiContext *pContext) {
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::Begin("RbxStu::Render::UI::PagedWindow", nullptr, ImGuiWindowFlags_NoTitleBar);

        const auto currentPageIndex = this->m_dwCurrentPageIndex;
        this->RenderPageButtons();

        ImGui::PushStyleColor(ImGuiCol_Button, Color4::FromRGB(39.0f, 136.0f, 245.0f).ToImGuiVec4());
        if (this->m_dwCurrentPageIndex == this->m_pages.size() || this->m_dwCurrentPageIndex == 0 ||
            this->m_pages.at(this->m_dwCurrentPageIndex - 1).bIsStub) {
            // We cannot regress more on the button, thus we want to color it as such.
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Button, Color4::FromRGB(0.0f, 0.0f, 0.0f).ToImGuiVec4());
        }
        {
            auto canMove = !(this->m_dwCurrentPageIndex == this->m_pages.size() || this->m_dwCurrentPageIndex == 0 ||
                             this->m_pages.at(this->m_dwCurrentPageIndex - 1).bIsStub);
            if (canMove && ImGui::Button("<<") || !canMove && ImGui::Button("--"))
                // Right condition just for feedback.
                if (canMove)
                    this->SetCurrentPage(this->m_dwCurrentPageIndex - 1);
        }

        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("%s ", this->m_pages.at(currentPageIndex).szPageName.c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, Color4::FromRGB(39.0f, 136.0f, 245.0f).ToImGuiVec4());
        if (this->m_dwCurrentPageIndex == this->m_pages.size() ||
            this->m_pages.at(this->m_dwCurrentPageIndex + 1).bIsStub) {
            // We cannot regress more on the button, thus we want to color it as such.
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Button, Color4::FromRGB(0.0f, 0.0f, 0.0f).ToImGuiVec4());
        }
        {
            auto canMove = !(this->m_dwCurrentPageIndex == this->m_pages.size() ||
                             this->m_pages.at(this->m_dwCurrentPageIndex + 1).bIsStub);
            if (canMove && ImGui::Button(">>") || !canMove && ImGui::Button("--"))
                // Right condition just for feedback.
                if (canMove)
                    this->SetCurrentPage(this->m_dwCurrentPageIndex + 1);
        }

        Renderable::PushSeparator();

        this->m_pages.at(currentPageIndex).pageRenderer->Render(pContext);

        ImGui::End();

        Renderable::Render(pContext);
    }
} // namespace RbxStu::Render::UI
