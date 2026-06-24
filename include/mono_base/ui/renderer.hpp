#pragma once

#include <Windows.h>

#include <functional>
#include <memory>

#include "mono_base/ui/graphics_api.hpp"

namespace mono_base::ui {

class IRenderer {
public:
    using DrawFn = std::function<void()>;

    virtual ~IRenderer() = default;

    [[nodiscard]] virtual GraphicsApi api() const = 0;
    [[nodiscard]] virtual const char* name() const = 0;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    [[nodiscard]] virtual bool imgui_ready() const = 0;
    [[nodiscard]] virtual HWND hwnd() const = 0;

    virtual void on_frame(void* native, const DrawFn& draw) = 0;
    virtual void apply_input(bool menu_open) = 0;
};

[[nodiscard]] std::unique_ptr<IRenderer> create_renderer(GraphicsApi api);

} // namespace mono_base::ui
