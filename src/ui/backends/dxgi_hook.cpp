#include "mono_base/ui/backends/dxgi_hook.hpp"

#include <d3d11.h>
#include <dxgi.h>

#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"

namespace mono_base::ui::backends {

void* resolve_dxgi_present_target() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MonoBaseDxgiProbe";
    RegisterClassExW(&wc);

    HWND dummy = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = dummy;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swap = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL level{};
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    void* present_target = nullptr;
    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 1, D3D11_SDK_VERSION, &desc, &swap, &device, &level, &context))) {
        void** vtable = *reinterpret_cast<void***>(swap);
        present_target = vtable[8];
        swap->Release();
        device->Release();
        context->Release();
    } else {
        LOG_ERROR("DXGI", "Failed to create probe D3D11 device");
    }

    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return present_target;
}

bool install_dxgi_present_hook(void* detour, void** original) {
    void* target = resolve_dxgi_present_target();
    if (!target) {
        return false;
    }
    if (!hooks::HookManager::instance().create("DXGI.Present", target, detour, original)) {
        return false;
    }
    return hooks::HookManager::instance().enable("DXGI.Present");
}

void remove_dxgi_present_hook() {
    hooks::HookManager::instance().disable("DXGI.Present");
    hooks::HookManager::instance().remove("DXGI.Present");
}

} // namespace mono_base::ui::backends
