#include "../../rbx/update/offsets.hpp"
#include "../../rbx/update/globals.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include "render.hpp"
#include "menu/menu.h"
#include <mutex>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace render {
    using tPresent = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using tResizeBuffers = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    inline IDXGISwapChain* SwapChain = nullptr;
    inline ID3D11Device* D3D11Device = nullptr;
    inline ID3D11DeviceContext* D3D11Context = nullptr;
    inline ID3D11RenderTargetView* D3D11RenderTargetView = nullptr;

    inline tPresent PresentOrg = nullptr;
    inline tResizeBuffers ResizeBuffersOrg = nullptr;
    inline WNDPROC WndProcOrg = nullptr;

    inline bool UserInterface = exploit::internal::ui::enabled;
    inline float DpiScale = exploit::internal::ui::dpi;

    HRESULT __stdcall HookedPresent(IDXGISwapChain* InSwapChain, UINT SyncInterval, UINT Flags)
    {
        static std::once_flag call;
        std::call_once(call, [InSwapChain]() {
            ImGui::CreateContext();

            c_gui::styles();

            DXGI_SWAP_CHAIN_DESC SwapChainDesc;
            InSwapChain->GetDesc(&SwapChainDesc);

            HDC hdc = GetDC(SwapChainDesc.OutputWindow);
            int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(SwapChainDesc.OutputWindow, hdc);
            DpiScale = dpi / 96.0f;

            if (!D3D11RenderTargetView) {
                ID3D11Texture2D* Texture2D = nullptr;
                if (SUCCEEDED(InSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&Texture2D)))) {
                    D3D11Device->CreateRenderTargetView(Texture2D, nullptr, &D3D11RenderTargetView);
                    Texture2D->Release();
                }
            }

            ImGui_ImplWin32_Init(SwapChainDesc.OutputWindow);
            ImGui_ImplDX11_Init(D3D11Device, D3D11Context);

            ImFontConfig fontConfig{};
            fontConfig.SizePixels = 13.0f * DpiScale;
            ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

            ImGui::GetStyle().ScaleAllSizes(DpiScale);
            });

        if (!D3D11RenderTargetView) {
            ID3D11Texture2D* Texture2D = nullptr;
            if (SUCCEEDED(InSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&Texture2D)))) {
                D3D11Device->CreateRenderTargetView(Texture2D, nullptr, &D3D11RenderTargetView);
                Texture2D->Release();
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (UserInterface) /* for insert */
            c_gui::render(); /* yas */

        ImGui::Render();
        D3D11Context->OMSetRenderTargets(1, &D3D11RenderTargetView, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        return PresentOrg(InSwapChain, SyncInterval, Flags);
    }

    HRESULT __stdcall HookedResizeBuffers(IDXGISwapChain* InSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
    {
        if (D3D11RenderTargetView) {
            D3D11RenderTargetView->Release();
            D3D11RenderTargetView = nullptr;
        }
        return ResizeBuffersOrg(InSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }

    LRESULT __stdcall HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_KEYDOWN:
            if (wParam == VK_INSERT) {
                UserInterface = !UserInterface;
            }
            break;
        case WM_DPICHANGED:
            DpiScale = LOWORD(wParam) / 96.0f;
            break;
        }

        if (UserInterface && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
            return TRUE;
        }

        if (UserInterface) {
            switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
            case WM_SETCURSOR:
            case WM_INPUT:
                return TRUE;
            }
        }

        return CallWindowProcA(WndProcOrg, hWnd, msg, wParam, lParam);
    }

    void initialize() {
            uintptr_t VisualEngine = *reinterpret_cast<uintptr_t*>(Offsets::VisualEngine::Pointer);
            if (!VisualEngine) return;
            uintptr_t DeviceD3D11 = *reinterpret_cast<uintptr_t*>(VisualEngine + Offsets::VisualEngine::DeviceD3D11);
            if (!DeviceD3D11) return;
            SwapChain = *reinterpret_cast<IDXGISwapChain**>(DeviceD3D11 + Offsets::VisualEngine::SwapChain);
            if (!SwapChain) return;

            DXGI_SWAP_CHAIN_DESC SwapChainDesc;
            if (FAILED(SwapChain->GetDesc(&SwapChainDesc))) return;
            if (FAILED(SwapChain->GetDevice(__uuidof(ID3D11Device), (void**)(&D3D11Device))))
                MessageBoxA(NULL, "It looks like you aren't using DirectX, please set your preferred rendering mode to Direct3D 11 and relaunch Roblox.", "uh oh", MB_OK);
                return;

            D3D11Device->GetImmediateContext(&D3D11Context);

            constexpr size_t VftableSize = 18;
            void** Vftable = *reinterpret_cast<void***>(SwapChain);

            static void** ShadowVftable = new void* [VftableSize];
            memcpy(ShadowVftable, Vftable, sizeof(void*) * VftableSize);

            PresentOrg = reinterpret_cast<tPresent>(Vftable[8]);
            ResizeBuffersOrg = reinterpret_cast<tResizeBuffers>(Vftable[13]);

            ShadowVftable[8] = reinterpret_cast<void*>(&HookedPresent);
            ShadowVftable[13] = reinterpret_cast<void*>(&HookedResizeBuffers);

            *reinterpret_cast<void***>(SwapChain) = ShadowVftable;

            WndProcOrg = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(SwapChainDesc.OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
    }

    void shutdown() {
            if (SwapChain && PresentOrg) {
                void** Vftable = *reinterpret_cast<void***>(SwapChain);
                Vftable[8] = reinterpret_cast<void*>(PresentOrg);
                Vftable[13] = reinterpret_cast<void*>(ResizeBuffersOrg);
            }

            if (SwapChain) {
                DXGI_SWAP_CHAIN_DESC SwapChainDesc;
                if (SUCCEEDED(SwapChain->GetDesc(&SwapChainDesc))) {
                    SetWindowLongPtrA(SwapChainDesc.OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcOrg));
                }
            }

            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            if (D3D11RenderTargetView) {
                D3D11RenderTargetView->Release();
                D3D11RenderTargetView = nullptr;
            }
            if (D3D11Context) {
                D3D11Context->Release();
                D3D11Context = nullptr;
            }
            if (D3D11Device) {
                D3D11Device->Release();
                D3D11Device = nullptr;
            }
    }
}