#include "mono_base/ui/present_bridge.hpp"

namespace mono_base::ui {
namespace {

PresentBridgeFn g_present_bridge = nullptr;
SwapBridgeFn g_swap_bridge = nullptr;

} // namespace

void set_present_bridge(PresentBridgeFn fn) {
    g_present_bridge = fn;
}

void invoke_present_bridge(void* native) {
    if (g_present_bridge) {
        g_present_bridge(native);
    }
}

void set_swap_bridge(SwapBridgeFn fn) {
    g_swap_bridge = fn;
}

BOOL invoke_swap_bridge(void* native) {
    return g_swap_bridge ? g_swap_bridge(native) : TRUE;
}

} // namespace mono_base::ui
