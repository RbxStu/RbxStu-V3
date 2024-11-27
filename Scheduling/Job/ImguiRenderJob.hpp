//
// Created by Dottik on 7/11/2024.
//

#pragma once
#include <dxgi.h>
#include <Scheduling/Job.hpp>

namespace RbxStu::Scheduling::Jobs {
    class ImguiRenderJob final : public Job {
        static ImguiRenderJob *Singleton;

        std::atomic_bool m_bIsInitialized;

        static LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

        bool ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                        RBX::TaskScheduler::Job::Stats *jobStats) override;

        Jobs::AvailableJobs GetJobIdentifier() override { return Jobs::AvailableJobs::ImguiRenderJob; }

        void Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                  RbxStu::Scheduling::TaskScheduler *scheduler) override;
    };
}
