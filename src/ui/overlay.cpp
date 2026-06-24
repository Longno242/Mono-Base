#include "mono_base/ui/overlay.hpp"

#include <imgui.h>
#include <imgui_impl_win32.h>

#include <functional>

#include <dxgi.h>

#include "mono_base/core/config.hpp"
#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/ui/graphics_api.hpp"
#include "mono_base/ui/menu.hpp"
#include "mono_base/ui/overlay_common.hpp"
#include "mono_base/ui/present_bridge.hpp"
#include "mono_base/ui/renderer.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace mono_base::ui {

Overlay& Overlay::instance() {
    static Overlay overlay;
    return overlay;
}

void Overlay::set_render_callback(RenderCallback cb) {
    std::lock_guard lock(callback_mutex_);
    render_callback_ = std::move(cb);
}

void Overlay::replace_renderer(std::unique_ptr<IRenderer> renderer) {
    std::lock_guard lock(render_mutex_);
    replace_renderer_unlocked(std::move(renderer));
}

void Overlay::replace_renderer_unlocked(std::unique_ptr<IRenderer> renderer) {
    if (renderer_) {
        renderer_->shutdown();
    }
    renderer_ = std::move(renderer);
    if (renderer_ && !renderer_->init()) {
        LOG_ERROR("Overlay", "Failed to init %s renderer", renderer_->name());
    }
}

HWND Overlay::game_window() const {
    return renderer_ ? renderer_->hwnd() : nullptr;
}

bool Overlay::init() {
    if (initialized_.load()) {
        return true;
    }

    if (!hooks::HookManager::instance().init()) {
        return false;
    }

    set_present_bridge([](void* native) {
        Overlay::instance().handle_present(native);
    });
    set_swap_bridge([](void* native) -> BOOL {
        Overlay::instance().handle_swap(native);
        return TRUE;
    });

    renderer_ = create_renderer(config::kGraphicsApi);
    if (!renderer_->init()) {
        LOG_FATAL("Overlay", "Failed to initialize %s renderer", graphics_api_name(config::kGraphicsApi));
        return false;
    }

    initialized_.store(true);
    LOG_INFO("Overlay", "Using graphics API: %s", graphics_api_name(config::kGraphicsApi));
    return true;
}

void Overlay::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    HWND hwnd = game_window();
    if (hwnd && original_wndproc_) {
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original_wndproc_));
        original_wndproc_ = nullptr;
    }
    if (hwnd && GetCapture() == hwnd) {
        ReleaseCapture();
    }
    wndproc_hooked_.store(false);

    set_present_bridge(nullptr);
    set_swap_bridge(nullptr);

    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }

    initialized_.store(false);
}

void Overlay::handle_present(void* native) {
    if (!initialized_.load() || !renderer_) {
        return;
    }

    static thread_local bool in_frame = false;
    if (in_frame) {
        return;
    }
    in_frame = true;

    std::lock_guard lock(render_mutex_);

    HWND window = renderer_->hwnd();
    if (!window) {
        if (auto* swap = static_cast<IDXGISwapChain*>(native)) {
            DXGI_SWAP_CHAIN_DESC desc{};
            if (SUCCEEDED(swap->GetDesc(&desc))) {
                window = desc.OutputWindow;
            }
        }
    }
    if (window) {
        ensure_wndproc_hook();
    }

    tick_features();

    if (GetAsyncKeyState(config::kMenuToggleKey) & 1) {
        menu_visible_.store(!menu_visible_.load());
    }

    const bool menu_open = menu_visible_.load();
    const IRenderer::DrawFn draw = [this] { draw_menu_shell(); };

    if (!menu_open) {
        renderer_->on_frame(native, [](void) {});
        renderer_->apply_input(false);
        in_frame = false;
        return;
    }

    renderer_->on_frame(native, draw);
    renderer_->apply_input(true);

    in_frame = false;
}

void Overlay::handle_swap(void* native) {
    if (!initialized_.load() || !renderer_) {
        return;
    }

    static thread_local bool in_frame = false;
    if (in_frame) {
        return;
    }
    in_frame = true;

    std::lock_guard lock(render_mutex_);

    if (renderer_->hwnd()) {
        ensure_wndproc_hook();
    }

    tick_features();

    if (GetAsyncKeyState(config::kMenuToggleKey) & 1) {
        menu_visible_.store(!menu_visible_.load());
    }

    const bool menu_open = menu_visible_.load();
    const IRenderer::DrawFn draw = [this] { draw_menu_shell(); };

    if (!menu_open) {
        renderer_->on_frame(native, [](void) {});
        renderer_->apply_input(false);
        in_frame = false;
        return;
    }

    renderer_->on_frame(native, draw);
    renderer_->apply_input(true);

    in_frame = false;
}

void Overlay::draw_menu_shell() {
    ImGui::SetNextWindowSize(ImVec2(540, 440), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowFocus();
    if (ImGui::Begin(config::kMenuTitle, nullptr,
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.36f, 0.96f, 1.0f));
        ImGui::TextUnformatted(config::kMenuHeader);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextDisabled("| %s", renderer_ ? renderer_->name() : "no renderer");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        RenderCallback cb;
        {
            std::lock_guard cb_lock(callback_mutex_);
            cb = render_callback_;
        }
        if (cb) {
            cb();
        } else {
            draw_menu();
        }
    }
    ImGui::End();
}

void Overlay::ensure_wndproc_hook() {
    HWND hwnd = game_window();
    if (wndproc_hooked_.load() || !hwnd) {
        return;
    }

    original_wndproc_ = reinterpret_cast<wndproc_fn>(
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Overlay::hooked_wndproc)));
    wndproc_hooked_.store(true);
    LOG_DEBUG("Overlay", "WndProc hooked");
}

LRESULT WINAPI Overlay::hooked_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto& self = Overlay::instance();

    if (self.menu_visible_.load() && self.renderer_ && self.renderer_->imgui_ready()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
        if (is_input_message(msg)) {
            return 0;
        }
    }
    if (self.original_wndproc_) {
        return CallWindowProcW(self.original_wndproc_, hwnd, msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // namespace mono_base::ui
