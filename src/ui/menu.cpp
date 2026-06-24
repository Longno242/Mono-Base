#include "mono_base/ui/menu.hpp"

#include <cstring>

#include <imgui.h>

#include "mono_base/core/config.hpp"
#include "mono_base/features/example_hook.hpp"
#include "mono_base/mono/resolver.hpp"
#include "mono_base/ui/graphics_api.hpp"
#include "mono_base/ui/overlay.hpp"
#include "mono_base/mono/helpers.hpp"

namespace mono_base::ui {
namespace {

char g_lookup_assembly[128] = "Assembly-CSharp";
char g_lookup_namespace[128]{};
char g_lookup_class[128]{};
mono::Class* g_lookup_result = nullptr;
char g_lookup_static_field[128] = "instance";
char g_lookup_status[256] = "Enter assembly + class name, then click Lookup.";

void draw_home_tab() {
    ImGui::TextColored(ImVec4(0.55f, 0.36f, 0.96f, 1.0f), "Getting started");
    ImGui::Spacing();

    ImGui::TextWrapped(
        "This is a starter template for Mono Unity games. "
        "Edit include/mono_base/core/config.hpp for your game, "
        "then add features under src/features/.");
    ImGui::Spacing();

    ImGui::BulletText("DELETE — toggle menu");
    ImGui::BulletText("Hooks tab — example MinHook + mono method hook");
    ImGui::BulletText("Debug tab — browse assemblies and lookup classes");
    ImGui::Spacing();

    ImGui::Separator();
    ImGui::TextDisabled("Logs: MonoBase.log + debug console");
    ImGui::TextDisabled("Mono resolver: %s", mono::ready() ? "ready" : "not ready");
}

void draw_debug_tab() {
    ImGui::TextColored(ImVec4(0.55f, 0.36f, 0.96f, 1.0f), "Mono resolver");
    ImGui::Spacing();

    ImGui::Text("Status: %s", mono::ready() ? "ready" : "not ready");
    ImGui::Text("Assemblies: %zu", mono::images().size());

    if (const ui::Overlay* overlay = &ui::Overlay::instance(); overlay->renderer()) {
        ImGui::Text("Graphics: %s", overlay->renderer()->name());
    } else {
        ImGui::Text("Graphics: %s (configured)", ui::graphics_api_name(config::kGraphicsApi));
    }

    if (!mono::ready()) {
        ImGui::Spacing();
        ImGui::TextDisabled("Inject after the game loads its Mono runtime.");
        return;
    }

    ImGui::Spacing();
    if (ImGui::BeginChild("##assemblies", ImVec2(0, 140), true)) {
        for (const mono::Image* img : mono::images()) {
            ImGui::BulletText("%s", img->name().c_str());
        }
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.55f, 0.36f, 0.96f, 1.0f), "Class lookup");
    ImGui::Spacing();

    ImGui::InputText("Assembly", g_lookup_assembly, sizeof(g_lookup_assembly));
    ImGui::InputText("Namespace", g_lookup_namespace, sizeof(g_lookup_namespace));
    ImGui::InputText("Class", g_lookup_class, sizeof(g_lookup_class));

    if (ImGui::Button("Lookup", ImVec2(120, 0))) {
        g_lookup_result = nullptr;
        if (g_lookup_assembly[0] == '\0' || g_lookup_class[0] == '\0') {
            snprintf(g_lookup_status, sizeof(g_lookup_status),
                "Assembly and class name are required.");
        } else {
            g_lookup_result = mono::find_class(g_lookup_assembly, g_lookup_class, g_lookup_namespace);
            if (g_lookup_result) {
                snprintf(g_lookup_status, sizeof(g_lookup_status),
                    "Found %s%s%s",
                    g_lookup_namespace[0] ? g_lookup_namespace : "",
                    g_lookup_namespace[0] ? "." : "",
                    g_lookup_class);
            } else {
                snprintf(g_lookup_status, sizeof(g_lookup_status),
                    "Class not found in '%s'.", g_lookup_assembly);
            }
        }
    }

    ImGui::TextWrapped("%s", g_lookup_status);

    if (g_lookup_result) {
        ImGui::Spacing();
        ImGui::Text("Type: %s", g_lookup_result->describe().c_str());

        ImGui::Spacing();
        ImGui::Text("Static field probe");
        ImGui::InputText("Field name", g_lookup_static_field, sizeof(g_lookup_static_field));
        if (ImGui::Button("Read static ptr", ImVec2(140, 0))) {
            void* ptr = mono::static_object_ptr(g_lookup_result, g_lookup_static_field);
            if (ptr) {
                snprintf(g_lookup_status, sizeof(g_lookup_status),
                    "Static '%s' -> %p (%s)",
                    g_lookup_static_field, ptr, mono::object_label(ptr).c_str());
            } else {
                snprintf(g_lookup_status, sizeof(g_lookup_status),
                    "Could not read static field '%s'.", g_lookup_static_field);
            }
        }
        ImGui::TextWrapped("%s", g_lookup_status);

        ImGui::Spacing();
        if (ImGui::BeginChild("##class_info", ImVec2(0, 180), true)) {
            ImGui::Text("Fields");
            for (const mono::Field* field : g_lookup_result->fields()) {
                ImGui::BulletText("%s", field->describe().c_str());
            }

            ImGui::Spacing();
            ImGui::Text("Methods");
            for (const mono::Method* method : g_lookup_result->methods()) {
                ImGui::BulletText("%s", method->describe().c_str());
            }
        }
        ImGui::EndChild();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Helpers: mono::object_name, static_object_ptr, invoke_string, mem_read");
    ImGui::TextDisabled("See include/mono_base/mono/helpers.hpp");
}

} // namespace

void tick_features() {
    features::tick();
}

void draw_menu() {
    if (ImGui::BeginTabBar("##maintabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Home")) {
            draw_home_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hooks")) {
            features::draw_hooks_tab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Debug")) {
            draw_debug_tab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

} // namespace mono_base::ui
