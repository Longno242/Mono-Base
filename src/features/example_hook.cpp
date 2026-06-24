#include "mono_base/features/example_hook.hpp"

#include <atomic>
#include <cstring>

#include <imgui.h>

#include "mono_base/core/config.hpp"
#include "mono_base/core/hooks.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/mono/resolver.hpp"

namespace mono_base::features {
namespace {

std::atomic<bool> g_installed{ false };
std::atomic<bool> g_hook_enabled{ false };
std::atomic<std::uint64_t> g_call_count{ 0 };

mono::Method* g_target_method = nullptr;

using example_fn = void(*)();
example_fn g_original = nullptr;

bool game_config_ready() {
    return game::kExampleAssembly[0] != '\0' &&
           game::kExampleClass[0] != '\0' &&
           game::kExampleMethod[0] != '\0';
}

void hooked_example() {
    g_call_count.fetch_add(1, std::memory_order_relaxed);
    if (g_original) {
        g_original();
    }
}

} // namespace

bool install() {
    if (g_installed.load()) {
        return true;
    }

    if (!game_config_ready()) {
        LOG_INFO("ExampleHook", "Skipped — fill game::kExample* in config.hpp to enable");
        g_installed.store(true);
        return true;
    }

    if (!mono::ready()) {
        LOG_WARN("ExampleHook", "Mono resolver not ready");
        return false;
    }

    mono::Class* klass = mono::find_class(game::kExampleAssembly, game::kExampleClass, game::kExampleNamespace);
    if (!klass) {
        LOG_ERROR("ExampleHook", "Class not found: %s.%s",
            game::kExampleNamespace, game::kExampleClass);
        return false;
    }

    g_target_method = klass->method(game::kExampleMethod, game::kExampleMethodArgCount);
    if (!g_target_method || !g_target_method->native()) {
        LOG_ERROR("ExampleHook", "Method not found or not compiled: %s", game::kExampleMethod);
        return false;
    }

    if (!hooks::HookManager::instance().create(
            "Example.TargetMethod",
            g_target_method->native(),
            &hooked_example,
            &g_original)) {
        return false;
    }

    if (!hooks::HookManager::instance().enable("Example.TargetMethod")) {
        hooks::HookManager::instance().remove("Example.TargetMethod");
        return false;
    }

    g_hook_enabled.store(true);
    g_installed.store(true);
    LOG_INFO("ExampleHook", "Hooked %s::%s @ %p",
        game::kExampleClass, game::kExampleMethod, g_target_method->native());
    return true;
}

void shutdown() {
    if (!g_installed.load()) {
        return;
    }

    hooks::HookManager::instance().disable("Example.TargetMethod");
    hooks::HookManager::instance().remove("Example.TargetMethod");

    g_target_method = nullptr;
    g_original = nullptr;
    g_hook_enabled.store(false);
    g_installed.store(false);
}

void tick() {
    // Per-frame logic for your features goes here.
}

void draw_hooks_tab() {
    ImGui::TextColored(ImVec4(0.55f, 0.36f, 0.96f, 1.0f), "Example hook");
    ImGui::Spacing();

    const bool configured = game_config_ready();
    ImGui::Text("Config: %s", configured ? "filled in" : "not configured");
    ImGui::Text("Installed: %s", g_installed.load() ? "yes" : "no");
    ImGui::Text("Active: %s", g_hook_enabled.load() ? "yes" : "no");
    ImGui::Text("Calls: %llu", static_cast<unsigned long long>(g_call_count.load()));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!configured) {
        ImGui::TextWrapped(
            "Open include/mono_base/core/config.hpp and set game::kExampleAssembly, "
            "game::kExampleClass, game::kExampleMethod (and optional namespace / arg count). "
            "Rebuild and reinject.");
        ImGui::Spacing();
        ImGui::TextDisabled("The DXGI Present hook is always installed by the overlay.");
        return;
    }

    ImGui::Text("Assembly: %s", game::kExampleAssembly);
    ImGui::Text("Class: %s%s%s",
        game::kExampleNamespace[0] ? game::kExampleNamespace : "",
        game::kExampleNamespace[0] ? "." : "",
        game::kExampleClass);
    ImGui::Text("Method: %s (%d args)", game::kExampleMethod, game::kExampleMethodArgCount);

    if (g_target_method) {
        ImGui::Text("Native: %p", g_target_method->native());
    }

    ImGui::Spacing();

    if (!g_installed.load()) {
        if (ImGui::Button("Install hook", ImVec2(140, 0))) {
            install();
        }
    } else if (g_hook_enabled.load()) {
        if (ImGui::Button("Disable hook", ImVec2(140, 0))) {
            hooks::HookManager::instance().disable("Example.TargetMethod");
            g_hook_enabled.store(false);
        }
    } else {
        if (ImGui::Button("Enable hook", ImVec2(140, 0))) {
            if (hooks::HookManager::instance().enable("Example.TargetMethod")) {
                g_hook_enabled.store(true);
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset counter", ImVec2(140, 0))) {
        g_call_count.store(0);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Copy src/features/example_hook.cpp when adding new hooks.");
}

} // namespace mono_base::features
