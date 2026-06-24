#pragma once

#include <Windows.h>

namespace mono_base::ui {

using PresentBridgeFn = void(*)(void* native);

void set_present_bridge(PresentBridgeFn fn);
void invoke_present_bridge(void* native);

using SwapBridgeFn = BOOL(*)(void* native);
void set_swap_bridge(SwapBridgeFn fn);
BOOL invoke_swap_bridge(void* native);

} // namespace mono_base::ui
