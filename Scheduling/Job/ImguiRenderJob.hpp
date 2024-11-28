//
// Created by Dottik on 7/11/2024.
//

#pragma once
#include <dxgi.h>
#include <list>
#include <Scheduling/Job.hpp>

#include "Render/Renderable.hpp"

namespace RbxStu::Render::ImmediateGui {
    enum class VirtualKey;
}

namespace RbxStu::Scheduling::Jobs {
    class ImguiRenderJob final : public Job {
        static ImguiRenderJob *Singleton;

        std::atomic_bool m_bIsInitialized;
        std::list<std::shared_ptr<Render::Renderable> > m_renderList;

        static LRESULT InputHwndProcedure(HWND hWnd, UINT uMsg, WPARAM wPram, LPARAM lParam);

        static LRESULT ImGuiHwndProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        static HRESULT __stdcall hkPresent(IDXGISwapChain *pSwapchain,
                                           UINT dwSyncInterval,
                                           UINT dwFlags);

        static HRESULT __stdcall hkResizeBuffers(IDXGISwapChain *pSelf,
                                                 UINT dwBufferCount,
                                                 UINT dwWidth,
                                                 UINT dwHeight,
                                                 DXGI_FORMAT newFormat,
                                                 UINT dwSwapChainFlags);

    public:
        ImguiRenderJob();

        bool IsKeyDown(RbxStu::Render::ImmediateGui::VirtualKey key);


        void FireKeyEventToRenderableObjects(RbxStu::Render::ImmediateGui::VirtualKey key, bool bIsDown) const;

        bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                        RBX::TaskScheduler::Job::Stats *jobStats) override;

        Jobs::AvailableJobs GetJobIdentifier() override { return Jobs::AvailableJobs::ImguiRenderJob; }

        void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                  RbxStu::Scheduling::TaskScheduler *scheduler) override;
    };
}
