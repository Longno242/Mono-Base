#include "mono_base/ui/backends/renderer_factory.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <mutex>

#include <Windows.h>

#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/ui/overlay_common.hpp"
#include "mono_base/ui/present_bridge.hpp"
#include "mono_base/ui/theme.hpp"

namespace mono_base::ui::backends {
namespace {

using wgl_swap_buffers_fn = BOOL(WINAPI*)(HDC);

class OpenGlRenderer final : public IRenderer {
public:
    OpenGlRenderer() { g_self = this; }
    ~OpenGlRenderer() override {
        if (g_self == this) {
            g_self = nullptr;
        }
    }

    GraphicsApi api() const override { return GraphicsApi::OpenGL; }
    const char* name() const override { return "OpenGL"; }

    bool init() override {
        const HMODULE opengl = GetModuleHandleA("opengl32.dll");
        if (!opengl) {
            LOG_ERROR("OpenGL", "opengl32.dll not loaded");
            return false;
        }

        void* target = reinterpret_cast<void*>(GetProcAddress(opengl, "wglSwapBuffers"));
        if (!target) {
            LOG_ERROR("OpenGL", "wglSwapBuffers not found");
            return false;
        }

        if (!hooks::HookManager::instance().create("OpenGL.wglSwapBuffers", target,
            reinterpret_cast<void*>(&swap_hook),
            reinterpret_cast<void**>(&original_swap_))) {
            return false;
        }
        return hooks::HookManager::instance().enable("OpenGL.wglSwapBuffers");
    }

    void shutdown() override {
        hooks::HookManager::instance().disable("OpenGL.wglSwapBuffers");
        hooks::HookManager::instance().remove("OpenGL.wglSwapBuffers");
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

        auto* hdc = static_cast<HDC>(native);
        std::lock_guard lock(mutex_);

        HWND window = WindowFromDC(hdc);
        if (window) {
            hwnd_ = window;
        }

        if (!ensure_ready()) {
            in_frame = false;
            return;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        draw();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        in_frame = false;
    }

    void apply_input(bool menu_open) override {
        if (imgui_ready_.load()) {
            apply_menu_input_state(hwnd_, menu_open);
        }
    }

private:
    static BOOL WINAPI swap_hook(HDC hdc) {
        invoke_swap_bridge(hdc);
        return g_self ? g_self->original_swap_(hdc) : FALSE;
    }

    bool ensure_ready() {
        if (imgui_ready_.load()) {
            return true;
        }
        if (!hwnd_) {
            return false;
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;
        theme::apply_modern();

        ImGui_ImplWin32_Init(hwnd_);
        ImGui_ImplOpenGL3_Init("#version 130");
        imgui_ready_.store(true);
        LOG_INFO("OpenGL", "ImGui initialized");
        return true;
    }

    void release_imgui() {
        if (!imgui_ready_.load()) {
            return;
        }
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imgui_ready_.store(false);
    }

    static inline OpenGlRenderer* g_self = nullptr;

    wgl_swap_buffers_fn original_swap_ = nullptr;
    HWND hwnd_ = nullptr;
    std::mutex mutex_;
    std::atomic<bool> imgui_ready_{ false };
};

} // namespace

std::unique_ptr<IRenderer> create_opengl_renderer() {
    return std::make_unique<OpenGlRenderer>();
}

} // namespace mono_base::ui::backends
