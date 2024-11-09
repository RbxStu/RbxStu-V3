//
// Created by Dottik on 7/11/2024.
//

#include "ImguiRenderJob.hpp"

#include <Scheduling/TaskSchedulerOrchestrator.hpp>

namespace RbxStu::Scheduling::Jobs {
    void ImguiRenderJob::InitializeHooks() {
        /*
         *  CDXGISwapChain::Present ->
         *      48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 45 33 F6 44 89 44 24 ? 44 39 35 DD 86 0F 00 41 8B F8 8B F2 89 54 24 ? 48 8B D9 48 89 4C 24 ? C6 44 24
         *
         *  CDXGIFactory::CreateSwapChainForHwnd ->
         *      40 55 53 56 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4
         */
    }

    bool ImguiRenderJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                    RBX::TaskScheduler::Job::Stats *jobStats) {
        /*
         *   Requirements:
         *      - Must be Render Job
         *      -
         */

        if (jobKind != JobKind::RenderJob)
            return false;

        if (!this->m_bAreRenderHooksInitialized) {
            /*
             *  We must first install all related hooks into the BEAUTIFUL DXGI.
             *
             *  For rendering, we will use the classic MakeSureDudeDies method, it consists of initailizing the rendering via hooks,
             *  however we render on our Step method to be in sync with the current RenderJob.
             */

            this->InitializeHooks();
        }
    }

    void ImguiRenderJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                              RbxStu::Scheduling::TaskScheduler *scheduler) {
    }
}
