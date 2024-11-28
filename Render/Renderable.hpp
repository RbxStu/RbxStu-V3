//
// Created by Dottik on 26/11/2024.
//

#pragma once
#include <atomic>
#include <imgui_internal.h>
#include <chrono>

namespace RbxStu::Render {
    namespace ImmediateGui {
        enum class VirtualKey;
    }

    ///
    /// @brief Provides an interface to define an object the ImGuiRendererJob can render into the screen utilizing dear ImGui.
    ///
    /// @remarks There are no windows created by default, only the ImGui frame is declared, you may create your own when overriding this class, however it is discouraged, and a Renderable object must only do simple things
    /// unless it is a full-blown User Interface with only ImGui, in which it is justified, and you should look into RbxStu::Render::ImGui::* for utilities on writing such.
    class Renderable abstract {
        std::chrono::time_point<std::chrono::steady_clock> m_lastFrame;
        bool m_bIsRenderingEnabled = false;

    protected:
        /*
         *  Lower level primitives for Drawing
         *      ~ Contains code from land/landervander ~
         */

        float GetDeltaTime(ImGuiContext *pContext) const;

        void PushSeparator();

    public:
        virtual ~Renderable() = default;

        void DisableRender();

        void EnableRender();

        bool IsRenderingEnabled();

        /// @brief Ran for rendering by RbxStu's Rendering Hook.
        /// @remarks The hook currently used for rendering is based on Kiero,
        /// with D3D11 as its rendering API, do NOT use any other API to draw to the screen.
        virtual void Render(ImGuiContext *pContext);

        /// @brief Ran by the RbxStu Window Procedure hook when a virtual key is pressed down.
        virtual void OnKeyPressed(ImmediateGui::VirtualKey key);

        /// @brief Ran by the RbxStu Window Procedure hook when a virtual key is released.
        virtual void OnKeyReleased(ImmediateGui::VirtualKey key);
    };
}
