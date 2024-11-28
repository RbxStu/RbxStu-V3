//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Render/Renderable.hpp"

struct ImGuiContext;

namespace RbxStu::Render::UI {
    struct UIPage {
        std::shared_ptr<RbxStu::Render::Renderable> pageRenderer;
        std::string szPageName;
    };

    class PagedWindow final : public RbxStu::Render::Renderable {
        std::string m_szWindowName;
        int m_currentPageIndex;
        std::vector<UIPage> m_pages;

    public:
        explicit PagedWindow(const std::vector<UIPage> &pages,
                             const std::string &szWindowName);

        ~PagedWindow() override;

        const UIPage &GetCurrentPage() const;

        void SetCurrentPage(const int newCurrentPage);

        void RenderPageButtons();

        void Render(ImGuiContext *pContext) override;;
    };
}
