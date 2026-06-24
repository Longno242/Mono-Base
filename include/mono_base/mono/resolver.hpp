#pragma once

#include <string>
#include <vector>

#include "mono_base/core/config.hpp"
#include "mono_base/core/logger.hpp"
#include "mono_base/mono/resolver/detail/api.hpp"
#include "mono_base/mono/resolver/types.hpp"
#include "mono_base/mono/resolver/types.inl.hpp"
#include "mono_base/mono/helpers.hpp"

namespace mono_base::mono {

[[nodiscard]] inline bool init() {
    auto& c = detail::ctx();
    if (c.initialized) {
        return true;
    }

    c.module = GetModuleHandleA(config::kMonoModulePrimary);
    c.module_name = config::kMonoModulePrimary;

    if (!c.module) {
        c.module = GetModuleHandleA(config::kMonoModuleFallback);
        c.module_name = config::kMonoModuleFallback;
    }

    if (!c.module) {
        LOG_FATAL("Mono", "Could not find mono runtime module");
        return false;
    }

    LOG_INFO("Mono", "Using module: %s", c.module_name);

    if (!detail::resolve_api()) {
        return false;
    }

    c.domain = c.api.get_root_domain();
    if (!c.domain) {
        LOG_FATAL("Mono", "mono_get_root_domain returned null");
        return false;
    }

    c.api.thread_attach(c.domain);
    detail::enumerate_assemblies();

    c.initialized = true;
    LOG_INFO("Mono", "Resolver ready — %zu assemblies loaded", c.images.size());
    return true;
}

[[nodiscard]] inline bool ready() {
    return detail::ctx().initialized;
}

[[nodiscard]] inline MonoDomain* domain() {
    return detail::ctx().domain;
}

[[nodiscard]] inline Image* image(const std::string& name) {
    auto& c = detail::ctx();
    if (!c.initialized) {
        return nullptr;
    }
    const auto it = c.image_lookup.find(name);
    return it != c.image_lookup.end() ? it->second : nullptr;
}

[[nodiscard]] inline const std::vector<Image*>& images() {
    static std::vector<Image*> result;
    auto& c = detail::ctx();

    if (!c.initialized) {
        result.clear();
        return result;
    }

    if (result.size() != c.images.size()) {
        result.clear();
        result.reserve(c.images.size());
        for (auto& img : c.images) {
            result.push_back(img.get());
        }
    }

    return result;
}

[[nodiscard]] inline Class* find_class(const std::string& image_name,
                                       const std::string& class_name,
                                       const std::string& namespaze = "") {
    Image* img = image(image_name);
    return img ? img->klass(class_name, namespaze) : nullptr;
}

} // namespace mono_base::mono
