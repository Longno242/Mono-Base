#include "mono_base/ui/backends/renderer_factory.hpp"

#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/ui/present_bridge.hpp"

#if defined(MONO_BASE_HAS_VULKAN)
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <mutex>

#include <vulkan/vulkan.h>

#include "mono_base/ui/overlay_common.hpp"
#include "mono_base/ui/theme.hpp"
#endif

namespace mono_base::ui::backends {
namespace {

#if defined(MONO_BASE_HAS_VULKAN)

using queue_present_fn = VkResult(VKAPI_PTR)(VkQueue, const VkPresentInfoKHR*);

VkInstance g_instance = VK_NULL_HANDLE;
VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
VkDevice g_device = VK_NULL_HANDLE;
VkQueue g_queue = VK_NULL_HANDLE;
uint32_t g_queue_family = 0;
VkFormat g_swap_format = VK_FORMAT_UNDEFINED;
uint32_t g_image_count = 0;

queue_present_fn g_original_present = nullptr;

class VulkanRenderer final : public IRenderer {
public:
    VulkanRenderer() { g_self = this; }
    ~VulkanRenderer() override {
        if (g_self == this) {
            g_self = nullptr;
        }
    }

    GraphicsApi api() const override { return GraphicsApi::Vulkan; }
    const char* name() const override { return "Vulkan"; }

    bool init() override {
        const HMODULE module = GetModuleHandleA("vulkan-1.dll");
        if (!module) {
            LOG_ERROR("Vulkan", "vulkan-1.dll not loaded");
            return false;
        }

        void* target = reinterpret_cast<void*>(GetProcAddress(module, "vkQueuePresentKHR"));
        if (!target) {
            LOG_ERROR("Vulkan", "vkQueuePresentKHR not found");
            return false;
        }

        return hooks::HookManager::instance().create("Vulkan.QueuePresent", target,
            reinterpret_cast<void*>(&present_hook),
            reinterpret_cast<void**>(&g_original_present));
    }

    void shutdown() override {
        hooks::HookManager::instance().disable("Vulkan.QueuePresent");
        hooks::HookManager::instance().remove("Vulkan.QueuePresent");
        release_imgui();
    }

    bool imgui_ready() const override { return imgui_ready_.load(); }
    HWND hwnd() const override { return hwnd_; }

    void on_frame(void* native, const DrawFn& draw) override {
        (void)native;
        if (!imgui_ready_.load()) {
            LOG_DEBUG("Vulkan", "Waiting for device/queue capture — set GraphicsApi to Dx11/Dx12 if needed");
            return;
        }

        std::lock_guard lock(mutex_);
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        draw();
        ImGui::Render();
        // Full command-buffer render path is game-specific; extend here when needed.
    }

    void apply_input(bool menu_open) override {
        if (imgui_ready_.load()) {
            apply_menu_input_state(hwnd_, menu_open);
        }
    }

private:
    static VkResult VKAPI_PTR present_hook(VkQueue queue, const VkPresentInfoKHR* info) {
        (void)queue;
        (void)info;
        static std::atomic<bool> logged{ false };
        if (!logged.exchange(true)) {
            LOG_INFO("Vulkan", "Present hook active — extend vulkan_renderer.cpp for full ImGui setup");
        }
        return g_original_present ? g_original_present(queue, info) : VK_SUCCESS;
    }

    void release_imgui() {
        if (!imgui_ready_.load()) {
            return;
        }
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imgui_ready_.store(false);
    }

    static inline VulkanRenderer* g_self = nullptr;

    HWND hwnd_ = nullptr;
    std::mutex mutex_;
    std::atomic<bool> imgui_ready_{ false };
};

#else

class VulkanRenderer final : public IRenderer {
public:
    GraphicsApi api() const override { return GraphicsApi::Vulkan; }
    const char* name() const override { return "Vulkan (not built)"; }
    bool init() override {
        LOG_ERROR("Vulkan", "Rebuild with Vulkan SDK installed (MONO_BASE_HAS_VULKAN)");
        return false;
    }
    void shutdown() override {}
    bool imgui_ready() const override { return false; }
    HWND hwnd() const override { return nullptr; }
    void on_frame(void*, const DrawFn&) override {}
    void apply_input(bool) override {}
};

#endif

} // namespace

std::unique_ptr<IRenderer> create_vulkan_renderer() {
    return std::make_unique<VulkanRenderer>();
}

} // namespace mono_base::ui::backends
