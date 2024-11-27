//
// Created by Dottik on 26/11/2024.
//

#pragma once
#include "imgui_internal.h"

namespace RbxStu::Render {
    class Renderable abstract {
    public:
        virtual ~Renderable() = default;

        virtual void Render(ImGuiContext *pContext);

        virtual void OnKeyPressed();
    };
}
