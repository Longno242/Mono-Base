#include "mono_base/ui/renderer.hpp"

#include "mono_base/ui/backends/renderer_factory.hpp"

namespace mono_base::ui {

std::unique_ptr<IRenderer> create_renderer(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::Auto:
        return backends::create_auto_renderer();
    case GraphicsApi::Dx11:
        return backends::create_dx11_renderer();
    case GraphicsApi::Dx12:
        return backends::create_dx12_renderer();
    case GraphicsApi::OpenGL:
        return backends::create_opengl_renderer();
    case GraphicsApi::Vulkan:
        return backends::create_vulkan_renderer();
    }
    return backends::create_auto_renderer();
}

} // namespace mono_base::ui
