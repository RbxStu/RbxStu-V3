//
// Created by Dottik on 27/11/2024.
//

#pragma once
#include "Render/Renderable.hpp"
#include "Roblox/TypeDefinitions.hpp"

namespace RbxStu::Render::UI::Pages {
    class ExecutionPage final : public RbxStu::Render::Renderable {
        std::string m_executeTextBuffer;
        std::vector<const char *> m_executionSecurities;
        int m_dwCurrentlySelectedExecutionSecurity;
        std::vector<const char *> m_executionDataModels;
        int m_dwCurrentlySelectedExecutionDataModel;

    public:
        ExecutionPage();

        void ExecuteBuffer();

        ~ExecutionPage() override = default;

        void Render(ImGuiContext *pContext) override;
    };
}
