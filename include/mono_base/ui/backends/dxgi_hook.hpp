#pragma once

namespace mono_base::ui::backends {

// Resolve IDXGISwapChain::Present from a temporary D3D11 swap chain.
[[nodiscard]] void* resolve_dxgi_present_target();

[[nodiscard]] bool install_dxgi_present_hook(void* detour, void** original);
void remove_dxgi_present_hook();

} // namespace mono_base::ui::backends
