#include "mono_base/ui/backends/renderer_factory.hpp"

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>

#include "mono_base/core/logger.hpp"
#include "mono_base/ui/backends/dxgi_hook.hpp"
#include "mono_base/ui/overlay.hpp"
#include "mono_base/ui/present_bridge.hpp"

namespace mono_base::ui::backends {
namespace {

class AutoRenderer final : public IRenderer {
public:
    GraphicsApi api() const override { return GraphicsApi::Auto; }
    const char* name() const override { return "Auto (DXGI)"; }

    bool init() override {
        return install_dxgi_present_hook(reinterpret_cast<void*>(&present_hook),
            reinterpret_cast<void**>(&original_present_));
    }

    void shutdown() override {
        remove_dxgi_present_hook();
    }

    bool imgui_ready() const override { return false; }
    HWND hwnd() const override { return nullptr; }

    void on_frame(void* native, const DrawFn& draw) override {
        auto* swap = static_cast<IDXGISwapChain*>(native);

        ID3D11Device* d11 = nullptr;
        if (SUCCEEDED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&d11)))) {
            d11->Release();
            LOG_INFO("Auto", "Detected DirectX 11");
            Overlay::instance().replace_renderer_unlocked(create_dx11_renderer());
            if (Overlay::instance().renderer()) {
                Overlay::instance().renderer()->on_frame(native, draw);
            }
            return;
        }

        ID3D12Device* d12 = nullptr;
        if (SUCCEEDED(swap->GetDevice(IID_PPV_ARGS(&d12)))) {
            d12->Release();
            LOG_INFO("Auto", "Detected DirectX 12");
            Overlay::instance().replace_renderer_unlocked(create_dx12_renderer());
            if (Overlay::instance().renderer()) {
                Overlay::instance().renderer()->on_frame(native, draw);
            }
            return;
        }

        LOG_DEBUG("Auto", "Waiting for D3D11/D3D12 swap chain");
    }

    void apply_input(bool) override {}

private:
    using present_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);

    static HRESULT WINAPI present_hook(IDXGISwapChain* swap, UINT sync, UINT flags) {
        invoke_present_bridge(swap);
        return g_self ? g_self->original_present_(swap, sync, flags) : S_OK;
    }

    static inline AutoRenderer* g_self = nullptr;

    present_fn original_present_ = nullptr;

public:
    AutoRenderer() { g_self = this; }
    ~AutoRenderer() override {
        if (g_self == this) {
            g_self = nullptr;
        }
    }
};

} // namespace

std::unique_ptr<IRenderer> create_auto_renderer() {
    return std::make_unique<AutoRenderer>();
}

} // namespace mono_base::ui::backends
