#include "mono_base/ui/backends/renderer_factory.hpp"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <mutex>

#include <d3d12.h>
#include <dxgi1_4.h>

#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/ui/backends/dxgi_hook.hpp"
#include "mono_base/ui/overlay_common.hpp"
#include "mono_base/ui/present_bridge.hpp"
#include "mono_base/ui/theme.hpp"

namespace mono_base::ui::backends {
namespace {

ID3D12CommandQueue* g_captured_queue = nullptr;

using CreateSwapChainForHwndFn = HRESULT(WINAPI*)(
    IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);

CreateSwapChainForHwndFn g_original_create_swapchain_for_hwnd = nullptr;

HRESULT WINAPI hooked_create_swapchain_for_hwnd(
    IDXGIFactory2* factory, IUnknown* device, HWND hwnd,
    const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreen,
    IDXGIOutput* output, IDXGISwapChain1** swap_chain) {
    if (device) {
        ID3D12CommandQueue* queue = nullptr;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&queue)))) {
            if (g_captured_queue) {
                g_captured_queue->Release();
            }
            g_captured_queue = queue;
            LOG_DEBUG("DX12", "Captured ID3D12CommandQueue from swap chain creation");
        }
    }
    return g_original_create_swapchain_for_hwnd(
        factory, device, hwnd, desc, fullscreen, output, swap_chain);
}

bool install_swapchain_create_hook() {
    IDXGIFactory2* factory = nullptr;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
        LOG_WARN("DX12", "CreateDXGIFactory2 failed — command queue capture unavailable");
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(factory);
    void* target = vtable[15];

    const bool ok = hooks::HookManager::instance().create(
        "DXGI.CreateSwapChainForHwnd", target,
        reinterpret_cast<void*>(&hooked_create_swapchain_for_hwnd),
        reinterpret_cast<void**>(&g_original_create_swapchain_for_hwnd));
    if (ok) {
        hooks::HookManager::instance().enable("DXGI.CreateSwapChainForHwnd");
    }
    factory->Release();
    return ok;
}

void remove_swapchain_create_hook() {
    hooks::HookManager::instance().disable("DXGI.CreateSwapChainForHwnd");
    hooks::HookManager::instance().remove("DXGI.CreateSwapChainForHwnd");
    if (g_captured_queue) {
        g_captured_queue->Release();
        g_captured_queue = nullptr;
    }
}

class Dx12Renderer final : public IRenderer {
public:
    Dx12Renderer() { g_self = this; }
    ~Dx12Renderer() override {
        if (g_self == this) {
            g_self = nullptr;
        }
    }

    GraphicsApi api() const override { return GraphicsApi::Dx12; }
    const char* name() const override { return "DirectX 12"; }

    bool init() override {
        install_swapchain_create_hook();
        return install_dxgi_present_hook(reinterpret_cast<void*>(&present_hook),
            reinterpret_cast<void**>(&original_present_));
    }

    void shutdown() override {
        remove_dxgi_present_hook();
        remove_swapchain_create_hook();
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

        auto* swap = static_cast<IDXGISwapChain3*>(native);
        std::lock_guard lock(mutex_);

        DXGI_SWAP_CHAIN_DESC desc{};
        if (SUCCEEDED(swap->GetDesc(&desc)) && desc.OutputWindow) {
            hwnd_ = desc.OutputWindow;
        }

        if (!ensure_ready(swap)) {
            in_frame = false;
            return;
        }

        frame_index_ = swap->GetCurrentBackBufferIndex();
        auto* back_buffer = render_targets_[frame_index_];
        if (!back_buffer) {
            in_frame = false;
            return;
        }

        allocators_[frame_index_]->Reset();
        command_list_->Reset(allocators_[frame_index_], nullptr);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = back_buffer;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        command_list_->ResourceBarrier(1, &barrier);

        const D3D12_CPU_DESCRIPTOR_HANDLE rtv = {
            rtv_heap_->GetCPUDescriptorHandleForHeapStart().ptr +
            static_cast<SIZE_T>(frame_index_) * rtv_descriptor_size_
        };
        command_list_->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        command_list_->SetDescriptorHeaps(1, &srv_heap_);

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        draw();
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), command_list_);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        command_list_->ResourceBarrier(1, &barrier);
        command_list_->Close();

        ID3D12CommandList* lists[] = { command_list_ };
        command_queue_->ExecuteCommandLists(1, lists);
        wait_for_gpu();

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

    static void alloc_srv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu,
                          D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
        if (!g_self) {
            return;
        }
        cpu->ptr = g_self->srv_cpu_start_.ptr + g_self->srv_alloc_next_;
        gpu->ptr = g_self->srv_gpu_start_.ptr + g_self->srv_alloc_next_;
        g_self->srv_alloc_next_ += g_self->srv_descriptor_size_;
    }

    static void free_srv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE,
                         D3D12_GPU_DESCRIPTOR_HANDLE) {}

    bool ensure_ready(IDXGISwapChain* swap) {
        ID3D12Device* swap_device = nullptr;
        if (FAILED(swap->GetDevice(IID_PPV_ARGS(&swap_device)))) {
            return false;
        }

        if (imgui_ready_.load() && device_ && device_ != swap_device) {
            LOG_INFO("DX12", "Device changed — reinitializing ImGui");
            release_imgui();
        }
        swap_device->Release();

        if (!command_queue_ && g_captured_queue) {
            command_queue_ = g_captured_queue;
            command_queue_->AddRef();
        }

        return imgui_ready_.load() ? true : setup_imgui(swap);
    }

    bool setup_imgui(IDXGISwapChain* swap) {
        if (FAILED(swap->GetDevice(IID_PPV_ARGS(&device_)))) {
            return false;
        }
        if (!command_queue_) {
            LOG_WARN("DX12", "Command queue not captured yet — create/wait for game swap chain");
            return false;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        swap->GetDesc(&desc);
        hwnd_ = desc.OutputWindow;
        buffer_count_ = desc.BufferCount;
        frame_index_ = 0;
        rtv_format_ = desc.BufferDesc.Format;

        D3D12_DESCRIPTOR_HEAP_DESC srv_desc{};
        srv_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_desc.NumDescriptors = 64;
        srv_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(device_->CreateDescriptorHeap(&srv_desc, IID_PPV_ARGS(&srv_heap_)))) {
            return false;
        }
        srv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        srv_cpu_start_ = srv_heap_->GetCPUDescriptorHandleForHeapStart();
        srv_gpu_start_ = srv_heap_->GetGPUDescriptorHandleForHeapStart();
        srv_alloc_next_ = 0;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        theme::apply_modern();

        ImGui_ImplWin32_Init(hwnd_);

        ImGui_ImplDX12_InitInfo init_info{};
        init_info.Device = device_;
        init_info.CommandQueue = command_queue_;
        init_info.NumFramesInFlight = static_cast<int>(buffer_count_);
        init_info.RTVFormat = rtv_format_;
        init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
        init_info.SrvDescriptorHeap = srv_heap_;
        init_info.SrvDescriptorAllocFn = &Dx12Renderer::alloc_srv;
        init_info.SrvDescriptorFreeFn = &Dx12Renderer::free_srv;
        if (!ImGui_ImplDX12_Init(&init_info)) {
            return false;
        }

        if (!create_render_targets(swap)) {
            return false;
        }

        imgui_ready_.store(true);
        LOG_INFO("DX12", "ImGui initialized");
        return true;
    }

    bool create_render_targets(IDXGISwapChain* swap) {
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap_desc.NumDescriptors = buffer_count_;
        if (FAILED(device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&rtv_heap_)))) {
            return false;
        }
        rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) {
            return false;
        }
        fence_value_ = 1;
        fence_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        for (UINT i = 0; i < buffer_count_; ++i) {
            if (FAILED(device_->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators_[i])))) {
                return false;
            }
            if (FAILED(swap->GetBuffer(i, IID_PPV_ARGS(&render_targets_[i])))) {
                return false;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE handle = {
                rtv_heap_->GetCPUDescriptorHandleForHeapStart().ptr +
                static_cast<SIZE_T>(i) * rtv_descriptor_size_
            };
            device_->CreateRenderTargetView(render_targets_[i], nullptr, handle);
        }

        if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                allocators_[0], nullptr, IID_PPV_ARGS(&command_list_)))) {
            return false;
        }
        command_list_->Close();
        return true;
    }

    void wait_for_gpu() {
        if (!command_queue_ || !fence_ || !fence_event_) {
            return;
        }
        command_queue_->Signal(fence_, fence_value_);
        if (fence_->GetCompletedValue() < fence_value_) {
            fence_->SetEventOnCompletion(fence_value_, fence_event_);
            WaitForSingleObject(fence_event_, INFINITE);
        }
        ++fence_value_;
    }

    void release_imgui() {
        if (!imgui_ready_.load()) {
            return;
        }
        wait_for_gpu();
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        for (UINT i = 0; i < buffer_count_; ++i) {
            if (render_targets_[i]) { render_targets_[i]->Release(); render_targets_[i] = nullptr; }
            if (allocators_[i]) { allocators_[i]->Release(); allocators_[i] = nullptr; }
        }
        if (command_list_) { command_list_->Release(); command_list_ = nullptr; }
        if (rtv_heap_) { rtv_heap_->Release(); rtv_heap_ = nullptr; }
        if (srv_heap_) { srv_heap_->Release(); srv_heap_ = nullptr; }
        if (fence_) { fence_->Release(); fence_ = nullptr; }
        if (fence_event_) { CloseHandle(fence_event_); fence_event_ = nullptr; }
        if (command_queue_) { command_queue_->Release(); command_queue_ = nullptr; }
        if (device_) { device_->Release(); device_ = nullptr; }

        buffer_count_ = 0;
        imgui_ready_.store(false);
    }

    static inline Dx12Renderer* g_self = nullptr;

    present_fn original_present_ = nullptr;
    HWND hwnd_ = nullptr;
    ID3D12Device* device_ = nullptr;
    ID3D12CommandQueue* command_queue_ = nullptr;
    ID3D12GraphicsCommandList* command_list_ = nullptr;
    ID3D12DescriptorHeap* rtv_heap_ = nullptr;
    ID3D12DescriptorHeap* srv_heap_ = nullptr;
    ID3D12Resource* render_targets_[4]{};
    ID3D12CommandAllocator* allocators_[4]{};
    ID3D12Fence* fence_ = nullptr;
    HANDLE fence_event_ = nullptr;
    UINT64 fence_value_ = 0;
    UINT buffer_count_ = 0;
    UINT frame_index_ = 0;
    UINT rtv_descriptor_size_ = 0;
    UINT srv_descriptor_size_ = 0;
    UINT srv_alloc_next_ = 0;
    DXGI_FORMAT rtv_format_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_start_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_start_{};
    std::mutex mutex_;
    std::atomic<bool> imgui_ready_{ false };
};

} // namespace

std::unique_ptr<IRenderer> create_dx12_renderer() {
    return std::make_unique<Dx12Renderer>();
}

} // namespace mono_base::ui::backends
