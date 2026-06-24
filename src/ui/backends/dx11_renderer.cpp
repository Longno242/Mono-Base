#include "mono_base/ui/backends/renderer_factory.hpp"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <mutex>

#include <d3d11.h>
#include <dxgi.h>

#include "mono_base/core/logger.hpp"
#include "mono_base/ui/backends/dxgi_hook.hpp"
#include "mono_base/ui/d3d11_state.hpp"
#include "mono_base/ui/overlay_common.hpp"
#include "mono_base/ui/present_bridge.hpp"
#include "mono_base/ui/theme.hpp"

namespace mono_base::ui::backends {
namespace {

class Dx11Renderer final : public IRenderer {
public:
    Dx11Renderer() { g_self = this; }
    ~Dx11Renderer() override {
        if (g_self == this) {
            g_self = nullptr;
        }
    }

    GraphicsApi api() const override { return GraphicsApi::Dx11; }
    const char* name() const override { return "DirectX 11"; }

    bool init() override {
        return install_dxgi_present_hook(reinterpret_cast<void*>(&present_hook),
            reinterpret_cast<void**>(&original_present_));
    }

    void shutdown() override {
        remove_dxgi_present_hook();
        release_imgui();
    }

    bool imgui_ready() const override { return imgui_ready_.load(); }
    HWND hwnd() const override { return hwnd_; }

    void on_frame(void* native, const DrawFn& draw) override {
        static thread_local bool in_frame = false;
        if (in_frame) {
            return;
        }
        in_frame = true;

        auto* swap = static_cast<IDXGISwapChain*>(native);
        std::lock_guard lock(mutex_);

        DXGI_SWAP_CHAIN_DESC desc{};
        if (SUCCEEDED(swap->GetDesc(&desc)) && desc.OutputWindow) {
            hwnd_ = desc.OutputWindow;
        }

        if (!ensure_ready(swap)) {
            in_frame = false;
            return;
        }

        ID3D11Texture2D* back_buffer = nullptr;
        if (FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D),
                reinterpret_cast<void**>(&back_buffer)))) {
            release_imgui();
            in_frame = false;
            return;
        }

        if (rtv_) {
            rtv_->Release();
            rtv_ = nullptr;
        }
        device_->CreateRenderTargetView(back_buffer, nullptr, &rtv_);
        back_buffer->Release();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        draw();
        ImGui::Render();

        d3d11::StateBackup backup;
        backup.backup(context_);
        context_->OMSetRenderTargets(1, &rtv_, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        backup.restore(context_);

        in_frame = false;
    }

    void apply_input(bool menu_open) override {
        if (imgui_ready_.load()) {
            apply_menu_input_state(hwnd_, menu_open);
        }
    }

private:
    using present_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);

    static HRESULT WINAPI present_hook(IDXGISwapChain* swap, UINT sync, UINT flags) {
        invoke_present_bridge(swap);
        return g_self ? g_self->original_present_(swap, sync, flags) : S_OK;
    }

    bool ensure_ready(IDXGISwapChain* swap) {
        ID3D11Device* swap_device = nullptr;
        if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&swap_device)))) {
            return false;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        swap->GetDesc(&desc);

        if (imgui_ready_.load() && device_ && device_ != swap_device) {
            LOG_INFO("DX11", "Device changed — reinitializing ImGui");
            release_imgui();
        }
        if (imgui_ready_.load() && hwnd_ && hwnd_ != desc.OutputWindow) {
            LOG_INFO("DX11", "Window changed — reinitializing ImGui");
            release_imgui();
        }
        swap_device->Release();

        return imgui_ready_.load() ? true : setup_imgui(swap);
    }

    bool setup_imgui(IDXGISwapChain* swap) {
        if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device_)))) {
            return false;
        }
        device_->GetImmediateContext(&context_);

        DXGI_SWAP_CHAIN_DESC desc{};
        swap->GetDesc(&desc);
        hwnd_ = desc.OutputWindow;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        theme::apply_modern();

        ImGui_ImplWin32_Init(hwnd_);
        ImGui_ImplDX11_Init(device_, context_);
        imgui_ready_.store(true);
        LOG_INFO("DX11", "ImGui initialized");
        return true;
    }

    void release_imgui() {
        if (!imgui_ready_.load()) {
            return;
        }
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        if (rtv_) { rtv_->Release(); rtv_ = nullptr; }
        if (context_) { context_->Release(); context_ = nullptr; }
        if (device_) { device_->Release(); device_ = nullptr; }
        imgui_ready_.store(false);
    }

    static inline Dx11Renderer* g_self = nullptr;

    present_fn original_present_ = nullptr;
    HWND hwnd_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
    std::mutex mutex_;
    std::atomic<bool> imgui_ready_{ false };
};

} // namespace

std::unique_ptr<IRenderer> create_dx11_renderer() {
    return std::make_unique<Dx11Renderer>();
}

} // namespace mono_base::ui::backends
