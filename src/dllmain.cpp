#include <Windows.h>

#include <filesystem>
#include <string>

#include "mono_base/core/config.hpp"
#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/features/example_hook.hpp"
#include "mono_base/mono/resolver.hpp"
#include "mono_base/ui/menu.hpp"
#include "mono_base/ui/overlay.hpp"

namespace {

using namespace mono_base;

std::wstring log_path() {
    wchar_t module_path[MAX_PATH]{};
    GetModuleFileNameW(GetModuleHandleW(nullptr), module_path, MAX_PATH);
    return (std::filesystem::path(module_path).parent_path() / config::kLogFileName).wstring();
}

void wait_for_mono(DWORD timeout_ms = 120000) {
    const DWORD step = 250;
    DWORD waited = 0;
    while (waited < timeout_ms) {
        if (GetModuleHandleA(config::kMonoModulePrimary) ||
            GetModuleHandleA(config::kMonoModuleFallback)) {
            return;
        }
        Sleep(step);
        waited += step;
    }
}

DWORD WINAPI main_thread(LPVOID module) {
    log::Logger::instance().init(log_path());
    LOG_INFO("Core", "MonoBase loaded — console is live for errors");

    wait_for_mono();
    if (!GetModuleHandleA(config::kMonoModulePrimary) &&
        !GetModuleHandleA(config::kMonoModuleFallback)) {
        LOG_FATAL("Core", "Timed out waiting for mono runtime");
        FreeLibraryAndExitThread(static_cast<HMODULE>(module), 1);
        return 1;
    }

    if (!mono::init()) {
        LOG_FATAL("Core", "Mono resolver init failed");
    }

    ui::Overlay::instance().set_render_callback(ui::draw_menu);
    if (!ui::Overlay::instance().init()) {
        LOG_FATAL("Core", "Overlay init failed");
    }

    if (!features::install()) {
        LOG_WARN("Core", "Example hook install failed (see Hooks tab)");
    }

    LOG_INFO("Core", "Initialization complete");
    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        const HANDLE handle = CreateThread(nullptr, 0, main_thread, module, 0, nullptr);
        if (handle) {
            CloseHandle(handle);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        features::shutdown();
        mono_base::ui::Overlay::instance().shutdown();
        mono_base::hooks::HookManager::instance().shutdown();
        mono_base::log::Logger::instance().shutdown();
    }
    return TRUE;
}
