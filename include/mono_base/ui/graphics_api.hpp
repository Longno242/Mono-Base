#pragma once

namespace mono_base::ui {

enum class GraphicsApi {
    Auto,    // DXGI Present -> auto-detect D3D11 or D3D12
    Dx11,    // DXGI + D3D11 ImGui backend
    Dx12,    // DXGI + D3D12 ImGui backend
    OpenGL,  // wglSwapBuffers hook
    Vulkan,  // vkQueuePresentKHR hook (requires Vulkan SDK at build time)
};

[[nodiscard]] inline const char* graphics_api_name(GraphicsApi api) {
    switch (api) {
    case GraphicsApi::Auto:   return "Auto (DXGI)";
    case GraphicsApi::Dx11:   return "DirectX 11";
    case GraphicsApi::Dx12:   return "DirectX 12";
    case GraphicsApi::OpenGL: return "OpenGL";
    case GraphicsApi::Vulkan: return "Vulkan";
    }
    return "Unknown";
}

} // namespace mono_base::ui
