//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <array>
#include <Logger.hpp>
#include <memory>

namespace RbxStu::Render {
    class Renderable;
}

namespace RbxStu::Render::UI {
    template<int U>
    class PagedWindow final {
        int m_currentPageIndex;
        std::array<std::shared_ptr<RbxStu::Render::Renderable>, U> m_renderables;

    public:
        explicit PagedWindow(std::array<std::shared_ptr<RbxStu::Render::Renderable>, U> pages) {
            this->m_renderables = pages;
            this->m_currentPageIndex = 0;
        }

        ~PagedWindow() {
            this->m_renderables.clear();
        }

        std::shared_ptr<RbxStu::Render::Renderable> GetCurrentPage() {
            return this->m_renderables[this->m_currentPageIndex];
        }

        void SetCurrentPage(int newCurrentPage) {
            if (m_renderables.size() > newCurrentPage) {
                RbxStuLog(RbxStu::LogType::Warning, RbxStu::Graphics,
                          "PagedWindow::SetCurrentPage(): Attempted to set the current page into index outside of the available page set, request dropped");
                return;
            }

            this->m_currentPageIndex = newCurrentPage;
        }
    };
}
