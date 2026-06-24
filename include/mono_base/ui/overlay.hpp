#pragma once

#include <Windows.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#include "mono_base/core/config.hpp"
#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/ui/renderer.hpp"

namespace mono_base::ui {

using RenderCallback = std::function<void()>;

class Overlay {
public:
    static Overlay& instance();

    bool init();
    void shutdown();

    void set_render_callback(RenderCallback cb);
    void replace_renderer(std::unique_ptr<IRenderer> renderer);
    // Called by auto-detect while render mutex is already held.
    void replace_renderer_unlocked(std::unique_ptr<IRenderer> renderer);

    void handle_present(void* native);
    void handle_swap(void* native);

    [[nodiscard]] bool menu_visible() const { return menu_visible_.load(); }
    [[nodiscard]] HWND game_window() const;
    [[nodiscard]] IRenderer* renderer() const { return renderer_.get(); }

private:
    Overlay() = default;

    static LRESULT WINAPI hooked_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    void ensure_wndproc_hook();
    void draw_menu_shell();

    using wndproc_fn = LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM);

    wndproc_fn original_wndproc_ = nullptr;

    std::unique_ptr<IRenderer> renderer_;
    std::mutex render_mutex_;
    std::mutex callback_mutex_;
    RenderCallback render_callback_;
    std::atomic<bool> menu_visible_{ false };
    std::atomic<bool> wndproc_hooked_{ false };
    std::atomic<bool> initialized_{ false };
};

} // namespace mono_base::ui
