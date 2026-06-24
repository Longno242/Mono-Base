#pragma once

#include "mono_base/ui/renderer.hpp"

namespace mono_base::ui::backends {

[[nodiscard]] std::unique_ptr<IRenderer> create_dx11_renderer();
[[nodiscard]] std::unique_ptr<IRenderer> create_dx12_renderer();
[[nodiscard]] std::unique_ptr<IRenderer> create_opengl_renderer();
[[nodiscard]] std::unique_ptr<IRenderer> create_vulkan_renderer();
[[nodiscard]] std::unique_ptr<IRenderer> create_auto_renderer();

} // namespace mono_base::ui::backends
