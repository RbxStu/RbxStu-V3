//
// Created by Dottik on 7/11/2024.
//

#include "ImguiRenderJob.hpp"

#include <d3d11.h>
#include <kiero.h>
#include <Logger.hpp>
#include <Scheduling/TaskSchedulerOrchestrator.hpp>

#include "imgui_internal.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

#include "Roblox/DataModel.hpp"

namespace RbxStu::Scheduling::Jobs {
    ImguiRenderJob *ImguiRenderJob::Singleton;

    static HRESULT (__stdcall *originalPresent)(IDXGISwapChain *, UINT, UINT) = nullptr;

    static HRESULT (__stdcall *originalResizeBuffers)(IDXGISwapChain *pSelf, UINT BufferCount, UINT Width, UINT Height,
                                                      DXGI_FORMAT NewFormat,
                                                      UINT SwapChainFlags) = nullptr;

    static WNDPROC g_pOriginalProcedure;
    static ID3D11Device *g_pDevice = nullptr;
    static ID3D11DeviceContext *g_pContext = nullptr;
    static ID3D11RenderTargetView *g_pRenderTargetView = nullptr;
    static HWND g_hWnd;
    static IDXGISwapChain *g_pCurrentSwapchain;
    static bool g_bReleasedView = false;

    bool KeyListener[256] = {};

    LRESULT __stdcall ImguiRenderJob::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
            return true;

        switch (uMsg) {
            case WM_LBUTTONDOWN:
                KeyListener[VK_LBUTTON] = true;
                break;
            case WM_LBUTTONUP:
                KeyListener[VK_LBUTTON] = false;
                break;
            case WM_RBUTTONDOWN:
                KeyListener[VK_RBUTTON] = true;
                break;
            case WM_RBUTTONUP:
                KeyListener[VK_RBUTTON] = false;
                break;
            case WM_MBUTTONDOWN:
                KeyListener[VK_MBUTTON] = true;
                break;
            case WM_MBUTTONUP:
                KeyListener[VK_MBUTTON] = false;
                break;
            case WM_XBUTTONDOWN: {
                if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
                    KeyListener[VK_XBUTTON1] = true;
                } else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2) {
                    KeyListener[VK_XBUTTON2] = true;
                }
                break;
            }
            case WM_XBUTTONUP: {
                if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
                    KeyListener[VK_XBUTTON1] = false;
                } else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2) {
                    KeyListener[VK_XBUTTON2] = false;
                }
                break;
            }
            case WM_KEYDOWN:
                KeyListener[wParam] = true;
                break;
            case WM_KEYUP:
                KeyListener[wParam] = false;
                break;
            default:
                break;
        }

        return CallWindowProcW(g_pOriginalProcedure, hWnd, uMsg, wParam, lParam);
    }

    static void ReleaseRenderView() {
        if (!g_bReleasedView) {
            g_bReleasedView = true;
            if (g_pRenderTargetView != nullptr) {
                g_pRenderTargetView->Release();
                g_pRenderTargetView = nullptr;
            }
        }
    }

    void ReInitializeBuffers(IDXGISwapChain *pSwapchain, HWND hWnd, bool bSetTarget) {
        ID3D11Texture2D *pBuffer;
        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics,
                  std::format("Reinitializing buffers with HWND and SwapChain {} & {}", (void*)hWnd,(void*) pSwapchain
                  ));

        pSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBuffer));
        g_pDevice->CreateRenderTargetView(pBuffer, nullptr, &g_pRenderTargetView);
        pBuffer->Release();

        if (bSetTarget)
            g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

        g_bReleasedView = false;

        RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics,
                  "Buffers re-initialized");
    }

    void TransitionHwnd(IDXGISwapChain *pSwapchain, const HWND hWnd) {
        if (ImGui::GetCurrentContext() != nullptr) {
            RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics, "ImGui::Context available");
            if (ImGui::GetIO().BackendPlatformUserData != nullptr) {
                RbxStuLog(RbxStu::LogType::Debug, RbxStu::Graphics, "ImGui::Win32::Shutdown()");
                ImGui_ImplWin32_Shutdown();
            }

            ImGui_ImplWin32_Init(hWnd);
        }

        g_pCurrentSwapchain = pSwapchain;
    }

    static bool g_bInitializedImGuiHook = false;

    HRESULT ImguiRenderJob::hkPresent(IDXGISwapChain *pSwapchain, const UINT dwSyncInterval, const UINT dwFlags) {
        if (pSwapchain != g_pCurrentSwapchain)
            return originalPresent(pSwapchain, dwSyncInterval, dwFlags);

        if (g_pRenderTargetView == nullptr)
            return originalPresent(pSwapchain, dwSyncInterval, dwFlags);

        g_pContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        ImGui::Render();

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pContext->OMSetRenderTargets(0, nullptr, nullptr);

        return originalPresent(pSwapchain, dwSyncInterval, dwFlags);
    }

    HRESULT ImguiRenderJob::hkResizeBuffers(IDXGISwapChain *pSelf, UINT dwBufferCount, UINT dwWidth, UINT dwHeight,
                                            DXGI_FORMAT newFormat,
                                            UINT dwSwapChainFlags) {
        if (!IsWindow(g_hWnd)) {
            DXGI_SWAP_CHAIN_DESC sd;
            pSelf->GetDesc(&sd);
            TransitionHwnd(pSelf, g_hWnd);
            g_hWnd = sd.OutputWindow;
            g_pDevice = nullptr;
        }

        if (!g_bInitializedImGuiHook) {
            g_pOriginalProcedure = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hWnd, GWLP_WNDPROC,
                                                                              reinterpret_cast<LONG_PTR>(WndProc)));

            pSelf->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void **>(&g_pDevice));
            g_pDevice->GetImmediateContext(&g_pContext);

            ImGui::CreateContext();

            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(g_pDevice, g_pContext);

            g_bInitializedImGuiHook = true;

            auto hr = originalResizeBuffers(pSelf, dwBufferCount, dwWidth, dwHeight, newFormat,
                                            dwSwapChainFlags); // Skip resize.

            D3D11_VIEWPORT vp;
            vp.Width = dwWidth;
            vp.Height = dwHeight;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = 0;
            vp.TopLeftY = 0;
            g_pContext->RSSetViewports(1, &vp);
            return hr;
        }

        if (g_pCurrentSwapchain != pSelf)
            return originalResizeBuffers(pSelf, dwBufferCount, dwWidth, dwHeight, newFormat,
                                         dwSwapChainFlags);

        g_pContext->OMSetRenderTargets(0, nullptr, nullptr);
        ReleaseRenderView();

        const auto hResult = originalResizeBuffers(pSelf, dwBufferCount, dwWidth, dwHeight, newFormat,
                                                   dwSwapChainFlags);

        ReInitializeBuffers(pSelf, g_hWnd, true);

        if (ImGui::GetDrawData() != nullptr) {
            auto drawData = ImGui::GetDrawData();
            drawData->DisplayPos = ImVec2(0.0f, 0.0f);
            drawData->DisplaySize = ImVec2(dwWidth, dwHeight);
        }

        D3D11_VIEWPORT vp;
        vp.Width = dwWidth;
        vp.Height = dwHeight;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        g_pContext->RSSetViewports(1, &vp);

        return hResult;
    }

    ImguiRenderJob::ImguiRenderJob() {
        ImguiRenderJob::Singleton = this; // Required due to hooking, nasty shit.
        this->m_bIsInitialized = false;
    }

    bool ImguiRenderJob::ShouldStep(RbxStu::Scheduling::JobKind jobKind, void *job,
                                    RBX::TaskScheduler::Job::Stats *jobStats) {
        /*
         *   Requirements:
         *      - Must be Render Job
         */

        // RenderJob for Standalone will never change, as it is always open once created, thus it is it whom we will hijack.

        return jobKind == JobKind::RenderJob && RbxStu::Roblox::DataModel::FromJob(job)->GetDataModelType() ==
               RBX::DataModelType_Edit;
    }

    void ImguiRenderJob::Step(void *job, RBX::TaskScheduler::Job::Stats *jobStats,
                              RbxStu::Scheduling::TaskScheduler *scheduler) {
        if (!this->m_bIsInitialized) {
            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous,
                      std::format("Initializing DX3D11 hooks @ RenderJob {}", job));

            kiero::init(kiero::RenderType::D3D11);

            void *phkPresent = &originalPresent;
            void *phkResizeBuffers = &originalResizeBuffers;

            kiero::bind(8, static_cast<void **>(phkPresent), hkPresent);
            kiero::bind(13, static_cast<void **>(phkResizeBuffers), hkResizeBuffers);

            this->m_bIsInitialized = true;

            RbxStuLog(RbxStu::LogType::Warning, RbxStu::Anonymous, "Initialized Hooks.");
        }
    }
}
