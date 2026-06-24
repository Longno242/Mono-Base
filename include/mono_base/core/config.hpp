#pragma once

#include "mono_base/ui/graphics_api.hpp"

namespace mono_base::config {

// Unity mono module — try both names in resolver init.
inline constexpr const char* kMonoModulePrimary   = "mono-2.0-bdwgc.dll";
inline constexpr const char* kMonoModuleFallback  = "mono.dll";

inline constexpr const char* kLogFileName = "MonoBase.log";
inline constexpr int kMenuToggleKey = 0x2E; // DELETE

inline constexpr const char* kMenuTitle  = "MonoBase";
inline constexpr const char* kMenuHeader = "MONO BASE";

// Graphics API for the overlay hook + ImGui backend.
// Auto     — DXGI Present, auto-detect D3D11 vs D3D12 (default, most Unity games)
// Dx11     — force DirectX 11
// Dx12     — force DirectX 12
// OpenGL   — wglSwapBuffers hook (legacy OpenGL Unity)
// Vulkan   — vkQueuePresentKHR hook (requires Vulkan SDK at build time)
inline constexpr ui::GraphicsApi kGraphicsApi = ui::GraphicsApi::Auto;

} // namespace mono_base::config

// ---------------------------------------------------------------------------
// Your game — edit these when building a mod for a specific title.
// Leave kExample* empty to keep the example hook disabled.
// ---------------------------------------------------------------------------
namespace mono_base::game {

inline constexpr const char* kExampleAssembly = "";
inline constexpr const char* kExampleNamespace = "";
inline constexpr const char* kExampleClass = "";
inline constexpr const char* kExampleMethod = "";
inline constexpr int kExampleMethodArgCount = 0;

} // namespace mono_base::game
